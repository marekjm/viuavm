/*
 *  Copyright (C) 2022 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <elf.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <vector>

#include <viua/support/tty.h>


struct Fragment {
    size_t index{};
    std::optional<Elf64_Phdr> program_header{};
    Elf64_Shdr section_header{};
    std::vector<uint8_t> data;
};

struct Loaded_elf {
    Elf64_Ehdr header;
    std::vector<std::pair<std::string, Fragment>> fragments;

    auto find_fragment(std::string_view const) const
        -> std::optional<std::reference_wrapper<Fragment const>>;
    auto entry_point() const -> std::optional<size_t>;

    static auto fn_at(std::vector<uint8_t> const&, size_t const)
        -> std::pair<std::string, size_t>;
    auto name_function_at(size_t const offset) const
        -> std::pair<std::string, size_t>;
    auto function_table() const
        -> std::map<size_t, std::pair<std::string, size_t>>;

    static auto load(int const elf_fd) -> Loaded_elf;
};
auto Loaded_elf::load(int const elf_fd) -> Loaded_elf
{
    auto loaded = Loaded_elf{};
    read(elf_fd, &loaded.header, sizeof(loaded.header));

    auto const& ehdr = loaded.header;

    auto pheaders = std::vector<Elf64_Phdr>{};
    {
        lseek(elf_fd, ehdr.e_phoff, SEEK_SET);

        for (auto i = size_t{0}; i < ehdr.e_phnum; ++i) {
            pheaders.emplace_back(Elf64_Phdr{});
            read(elf_fd, &pheaders.back(), ehdr.e_phentsize);
        }
    }

    auto sheaders = std::vector<Elf64_Shdr>{};
    {
        lseek(elf_fd, ehdr.e_shoff, SEEK_SET);

        for (auto i = size_t{0}; i < ehdr.e_shnum; ++i) {
            sheaders.emplace_back(Elf64_Shdr{});
            read(elf_fd, &sheaders.back(), ehdr.e_shentsize);
        }
    }

    auto section_names = std::vector<std::string>{};
    {
        auto const& shstrtab = sheaders.back();

        auto buf = std::vector<char>(shstrtab.sh_size);
        lseek(elf_fd, shstrtab.sh_offset, SEEK_SET);
        read(elf_fd, buf.data(), buf.size());

        for (auto i = size_t{0}; i < shstrtab.sh_size; ++i) {
            section_names.emplace_back(&buf[i]);
            i += section_names.back().size();
        }
    }

    auto sh_index = size_t{0};
    for (auto const& sh : sheaders) {
        auto const ph = std::find_if(
            pheaders.begin(), pheaders.end(), [&sh](auto const& each) -> bool {
                return (each.p_offset == sh.sh_offset);
            });

        auto fragment = Fragment{};

        fragment.section_header = sh;

        if (ph != pheaders.end()) {
            fragment.program_header = *ph;
        }

        if (sh.sh_size) {
            fragment.data.resize(sh.sh_size);
            lseek(elf_fd, sh.sh_offset, SEEK_SET);
            read(elf_fd, fragment.data.data(), sh.sh_size);
        }

        fragment.index = sh_index++;
        loaded.fragments.push_back(
            {section_names.at(fragment.index), std::move(fragment)});
    }

    return loaded;
}
auto Loaded_elf::entry_point() const -> std::optional<size_t>
{
    if (auto const f = find_fragment(".text");
        header.e_entry and f.has_value()) {
        return (header.e_entry - f->get().program_header->p_offset);
    } else {
        return std::nullopt;
    }
}
auto Loaded_elf::find_fragment(std::string_view const sv) const
    -> std::optional<std::reference_wrapper<Fragment const>>
{
    auto const frag = std::find_if(
        fragments.begin(), fragments.end(), [sv](auto const& frag) -> bool {
            return (frag.first == sv);
        });
    return (frag == fragments.end()) ? std::nullopt
                                     : std::optional{std::ref(frag->second)};
}
auto Loaded_elf::fn_at(std::vector<uint8_t> const& function_table,
                       size_t const offset) -> std::pair<std::string, size_t>
{
    auto sz = uint64_t{};
    memcpy(&sz, (function_table.data() + offset - sizeof(sz)), sizeof(sz));
    sz = le64toh(sz);


    auto name = std::string{
        reinterpret_cast<char const*>(function_table.data()) + offset, sz};


    auto addr = uint64_t{};
    memcpy(&addr, (function_table.data() + offset + sz), sizeof(addr));
    addr = le64toh(addr);

    /*
    std::cout << "Offset:     " << offset << "\n";
    std::cout << "Label size: " << sz << "\n";
    std::cout << "Label:      " << name << "\n";
    std::cout << "Address:    " << addr << "\n";
    */

    return {name, addr};
}
auto Loaded_elf::name_function_at(size_t const offset) const
    -> std::pair<std::string, size_t>
{
    auto const& functions_table = find_fragment(".viua.fns")->get();
    return fn_at(functions_table.data, offset);
}
auto Loaded_elf::function_table() const
    -> std::map<size_t, std::pair<std::string, size_t>>
{
    auto const& raw = find_fragment(".viua.fns")->get();
    auto ft         = std::map<size_t, std::pair<std::string, size_t>>{};

    for (auto i = size_t{sizeof(uint64_t)}; i < raw.data.size();
         i += (2 * sizeof(uint64_t))) {
        auto fn = fn_at(raw.data, i);
        ft[i]   = std::move(fn);
        i += ft[i].first.size();
    }

    return ft;
}

constexpr auto VIUA_MAGIC
    [[maybe_unused]] = std::string_view{"\x7fVIUA\x00\x00\x00", 8};

auto main(int argc, char* argv[]) -> int
{
    using viua::support::tty::ATTR_RESET;
    using viua::support::tty::COLOR_FG_CYAN;
    using viua::support::tty::COLOR_FG_ORANGE_RED_1;
    using viua::support::tty::COLOR_FG_RED;
    using viua::support::tty::COLOR_FG_RED_1;
    using viua::support::tty::COLOR_FG_WHITE;
    using viua::support::tty::send_escape_seq;
    constexpr auto esc = send_escape_seq;

    if (argc == 1) {
        std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                  << ": not path to load\n";
        return 1;
    }

    auto const elf_path = std::filesystem::path{argv[1]};
    if (not std::filesystem::exists(elf_path)) {
        std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                  << ": file does not exist: " << esc(2, COLOR_FG_WHITE)
                  << elf_path.string() << esc(2, ATTR_RESET) << "\n";
        return 1;
    }

    auto const elf_fd = open(elf_path.c_str(), O_RDONLY);
    if (elf_fd == -1) {
        auto const saved_errno = errno;
        auto const errname     = strerrorname_np(saved_errno);
        auto const errdesc     = strerrordesc_np(saved_errno);

        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.string()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET);
        if (errname) {
            std::cerr << ": " << errname;
        }
        std::cerr << ": " << (errdesc ? errdesc : "unknown error") << "\n";
        return 1;
    }

    auto const elf = Loaded_elf::load(elf_fd);
    std::cout << "Fragments:\n";

    auto const index_width = std::to_string(elf.fragments.size()).size();
    for (auto const& [name, each] : elf.fragments) {
        auto const& sh [[maybe_unused]] = each.section_header;
        auto const& ph [[maybe_unused]] = each.program_header;

        if (sh.sh_type == SHT_NULL) {
            continue;
        }

        {
            constexpr auto NAME_WIDTH = 20;
            std::cout << "  [" << std::setw(index_width) << each.index << "] ";
            std::cout << name << std::string((NAME_WIDTH - name.size()), ' ');
            std::cout << ((sh.sh_type == SHT_NOBITS)     ? "NOBITS"
                          : (sh.sh_type == SHT_PROGBITS) ? "PROGBITS"
                          : (sh.sh_type == SHT_STRTAB)   ? "STRTAB"
                          : (sh.sh_type == SHT_NULL)
                              ? "NULL"
                              : "<unexpected section header type>");
            if (ph.has_value()) {
                std::cout << " in ";
                std::cout << ((ph->p_type == PT_LOAD)     ? "LOAD"
                              : (ph->p_type == PT_INTERP) ? "INTERP"
                              : (ph->p_type == PT_NULL)
                                  ? "NULL"
                                  : "<unexpected program header type>");
            }
            std::cout << "\n";
        }

        auto const INDENT = std::string((index_width + 5), ' ');
        {
            std::cout << INDENT << "Offset       "
                      << std::setw(sizeof(sh.sh_offset)) << std::setfill('0')
                      << std::hex << sh.sh_offset << std::dec << " ("
                      << sh.sh_offset << " bytes)"
                      << "\n";

            std::cout << INDENT << "File size    "
                      << std::setw(sizeof(sh.sh_size)) << std::setfill('0')
                      << std::hex << sh.sh_size << std::dec << " ("
                      << sh.sh_size << " bytes)"
                      << "\n";

            if (ph and (ph->p_type == PT_LOAD)) {
                std::cout << INDENT << "Memory size  "
                          << std::setw(sizeof(ph->p_memsz)) << std::setfill('0')
                          << std::hex << ph->p_memsz << std::dec << " ("
                          << ph->p_memsz << " bytes)"
                          << "\n";
            }
        }

        if (name == ".interp") {
            std::cout << INDENT << "  [Interpreter: " << each.data.data()
                      << "]\n";
        }
        if (name == ".viua.magic") {
            std::cout << INDENT << "  [Magic:";
            std::cout << std::hex;
            for (auto const each : each.data) {
                std::cout << ' ' << std::setw(2) << std::setfill('0')
                          << static_cast<int>(each);
            }
            std::cout << std::dec;

            std::cout << (((VIUA_MAGIC.size() == each.data.size())
                           and (memcmp(VIUA_MAGIC.data(),
                                       each.data.data(),
                                       VIUA_MAGIC.size())
                                == 0))
                              ? " (valid)"
                              : " (invalid)")
                      << "]\n";
        }
    }

    std::cout << "\nType:        "
              << ((elf.header.e_type == ET_EXEC) ? "EXEC (Executable)"
                                                 : "REL (Relocatable)")
              << "\n";
    std::cout << "Entry point: ";
    if (auto const ep = elf.entry_point(); ep.has_value()) {
        std::cout << std::setw(16) << std::setfill('0') << std::hex
                  << elf.header.e_entry << "  [.text+0x" << std::hex << *ep
                  << "]";
        std::cout << std::dec;
        std::cout << "  " << elf.name_function_at(*ep).first << "\n";
    } else {
        std::cout << "not found\n";
    }

    std::cout << "\nFunction table:\n";
    std::cout << "  " << std::setw(16) << std::setfill(' ') << "Label offset"
              << "            " << std::setw(16) << std::setfill(' ')
              << "Target address"
              << "  Label"
              << "\n";
    for (auto const& [offset, fn] : elf.function_table()) {
        auto const& [name, addr] = fn;

        std::cout << "  " << std::hex << std::setw(16) << std::setfill('0')
                  << offset << "  [.text+0x" << std::setw(16)
                  << std::setfill('0') << addr << "]  " << name << "\n";
    }

    return 0;
}
