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

#include <viua/vm/elf.h>


namespace viua::vm::elf {
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

auto Loaded_elf::make_text_from(Fragment::data_type const& data) -> std::vector<viua::arch::instruction_type>
{
    auto text = std::vector<viua::arch::instruction_type>{};
    text.reserve(data.size() / sizeof(viua::arch::instruction_type));
    for (auto i = size_t{0}; i < data.size();
         i += sizeof(viua::arch::instruction_type)) {
        text.emplace_back(viua::arch::instruction_type{});
        memcpy(&text.back(),
               &data[i],
               sizeof(viua::arch::instruction_type));
    }
    return text;
}
}  // namespace viua::vm::elf
