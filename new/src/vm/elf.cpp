/*
 *  Copyright (C) 2022-2023 Marek Marecki
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

#include <endian.h>

#include <iomanip>
#include <iostream>

#include <viua/support/fdio.h>
#include <viua/vm/elf.h>


namespace viua::vm::elf {
using viua::support::posix::whole_read;
using viua::support::posix::whole_write;

auto Loaded_elf::load(int const elf_fd) -> Loaded_elf
{
    auto loaded = Loaded_elf{};
    if (read(elf_fd, &loaded.header, sizeof(loaded.header))
        != sizeof(loaded.header)) {
        throw std::runtime_error{"could not read program header"};
    }

    auto const& ehdr = loaded.header;
    {
        /*
         * Verify magic.
         */
        auto const magic_leader = (ehdr.e_ident[EI_MAG0] == '\x7f');
        auto const magic_e      = (ehdr.e_ident[EI_MAG1] == 'E');
        auto const magic_l      = (ehdr.e_ident[EI_MAG2] == 'L');
        auto const magic_f      = (ehdr.e_ident[EI_MAG3] == 'F');
        auto const magic_is_ok =
            (magic_leader and magic_e and magic_l and magic_f);
        if (not magic_is_ok) {
            throw std::runtime_error{"invalid ELF magic"};
        }
    }
    {
        /*
         * Verify other ELF attributes.
         */
        if (ehdr.e_ident[EI_CLASS] != ELFCLASS64) {
            throw std::runtime_error{"invalid ELF class"};
        }
        if (ehdr.e_ident[EI_DATA] != ELFDATA2LSB) {
            throw std::runtime_error{"invalid ELF endianness"};
        }
        if (ehdr.e_ident[EI_VERSION] != EV_CURRENT) {
            throw std::runtime_error{
                "invalid ELF version in identification array"};
        }
        if (ehdr.e_ident[EI_OSABI] != ELFOSABI_STANDALONE) {
            throw std::runtime_error{"invalid ELF OS ABI"};
        }
        if (ehdr.e_ident[EI_ABIVERSION] != 0) {
            throw std::runtime_error{"invalid ELF ABI version"};
        }
        if (ehdr.e_machine != ET_NONE) {
            throw std::runtime_error{"invalid ELF machine"};
        }
        if (ehdr.e_version != EV_CURRENT) {
            throw std::runtime_error{"invalid ELF version"};
        }
        if (ehdr.e_flags != 0) {
            throw std::runtime_error{"invalid ELF flags"};
        }
    }

    auto pheaders = std::vector<Elf64_Phdr>{};
    {
        lseek(elf_fd, ehdr.e_phoff, SEEK_SET);

        for (auto i = size_t{0}; i < ehdr.e_phnum; ++i) {
            pheaders.emplace_back(Elf64_Phdr{});
            whole_read(elf_fd, &pheaders.back(), ehdr.e_phentsize);
        }
    }

    auto sheaders = std::vector<Elf64_Shdr>{};
    {
        lseek(elf_fd, ehdr.e_shoff, SEEK_SET);

        for (auto i = size_t{0}; i < ehdr.e_shnum; ++i) {
            sheaders.emplace_back(Elf64_Shdr{});
            whole_read(elf_fd, &sheaders.back(), ehdr.e_shentsize);
        }
    }

    auto section_names = std::vector<std::string>{};
    {
        auto const& shstrtab = sheaders.back();

        auto buf = std::vector<char>(shstrtab.sh_size);
        lseek(elf_fd, shstrtab.sh_offset, SEEK_SET);
        whole_read(elf_fd, buf.data(), buf.size());

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
            whole_read(elf_fd, fragment.data.data(), sh.sh_size);
        }

        fragment.index = sh_index++;
        loaded.fragments.push_back(
            {section_names.at(fragment.index), std::move(fragment)});
    }

    loaded.load_strtab();
    loaded.load_symtab();

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
    auto const fragment = std::find_if(
        fragments.begin(), fragments.end(), [sv](auto const& frag) -> bool {
            return (frag.first == sv);
        });
    return (fragment == fragments.end())
               ? std::nullopt
               : std::optional{std::ref(fragment->second)};
}
auto Loaded_elf::load_strtab() -> void
{
    auto elf_strtab = find_fragment(".strtab");
    if (not elf_strtab.has_value()) {
        /*
         * FIXME Without signalling failure LOUDLY here, there will be
         * mysterious errors encountered later.
         */
        return;
    }

    auto const strtab_size = elf_strtab->get().section_header.sh_size;
    strtab                 = std::string_view{
        reinterpret_cast<char const*>(elf_strtab->get().data.data()),
        strtab_size};
    auto const strtab_data =
        reinterpret_cast<char const*>(elf_strtab->get().data.data());
    for (auto i = size_t{0}; i < strtab_size; ++i) {
        auto sv = std::string_view{strtab_data + i};
        strtab_quick.emplace(i, sv);
        i += sv.size();
    }
}
auto Loaded_elf::str_at(size_t const off) const -> std::string_view
{
    auto name = std::string_view{};
    if (strtab_quick.count(off)) {
        name = strtab_quick.at(off);
    } else {
        name = strtab;
        name.remove_prefix(off);
        name = std::string_view{name.data()};
    }
    if (strtab.size() <= off) {
        abort();
    }
    return name;
}
auto Loaded_elf::load_symtab() -> void
{
    auto elf_symtab = find_fragment(".symtab");
    if (not elf_symtab.has_value()) {
        /*
         * FIXME Without signalling failure LOUDLY here, there will be
         * mysterious errors encountered later.
         */
        return;
    }

    auto const symtab_data = elf_symtab->get().data.data();
    auto const no_of_symtab_entries =
        elf_symtab->get().section_header.sh_size / sizeof(Elf64_Sym);
    for (auto i = size_t{0}; i < no_of_symtab_entries; ++i) {
        auto sym = Elf64_Sym{};
        memcpy(&sym, symtab_data + (i * sizeof(Elf64_Sym)), sizeof(Elf64_Sym));

        if (ELF64_ST_TYPE(sym.st_info) == STT_FUNC) {
            fn_map.emplace(str_at(sym.st_name), symtab.size());
        }
        symtab.push_back(sym);
    }
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
auto Loaded_elf::name_function_at(size_t const offset) const -> std::string_view
{
    for (auto const& sym : symtab) {
        if (ELF64_ST_TYPE(sym.st_info) != STT_FUNC) {
            continue;
        }

        if (sym.st_value != offset) {
            continue;
        }

        return str_at(sym.st_name);
    }
    return "";
}
auto Loaded_elf::function_table() const
    -> std::map<size_t, std::pair<std::string, size_t>>
{
    auto const& raw = find_fragment(".symtab")->get();
    auto ft         = std::map<size_t, std::pair<std::string, size_t>>{};

    for (auto i = size_t{sizeof(uint64_t)}; i < raw.data.size();
         i += (2 * sizeof(uint64_t))) {
        auto fn = fn_at(raw.data, i);
        ft[i]   = std::move(fn);
        i += ft[i].first.size();
    }

    return ft;
}
auto Loaded_elf::labels_table() const -> std::map<size_t, std::string>
{
    auto lt         = std::map<size_t, std::string>{};
    auto const& raw = find_fragment(".viua.labels");
    if (not raw.has_value()) {
        return lt;
    }

    auto const data = raw->get().data;
    for (auto i = size_t{sizeof(uint64_t)}; i < data.size();
         i += (2 * sizeof(uint64_t))) {
        auto sz = uint64_t{};
        memcpy(&sz, (data.data() + i - sizeof(sz)), sizeof(sz));
        sz = le64toh(sz);

        auto name =
            std::string{reinterpret_cast<char const*>(data.data() + i), sz};
        auto addr = uint64_t{};
        memcpy(&addr, (data.data() + i + sz), sizeof(addr));
        addr = le64toh(addr);

        lt[addr] = std::move(name);

        i += sz;
    }

    return lt;
}

auto Loaded_elf::make_text_from(Fragment::data_type const& data)
    -> std::vector<viua::arch::instruction_type>
{
    auto text = std::vector<viua::arch::instruction_type>{};
    text.reserve(data.size() / sizeof(viua::arch::instruction_type));
    for (auto i = size_t{0}; i < data.size();
         i += sizeof(viua::arch::instruction_type)) {
        text.emplace_back(viua::arch::instruction_type{});
        memcpy(&text.back(), &data[i], sizeof(viua::arch::instruction_type));
    }
    return text;
}
}  // namespace viua::vm::elf
