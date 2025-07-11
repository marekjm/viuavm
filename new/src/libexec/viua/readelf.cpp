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

#include <fcntl.h>

#include <filesystem>
#include <iostream>
#include <numeric>

#include <viua/arch/elf.h>
#include <viua/libexec/common.hh>
#include <viua/support/elf.hh>
#include <viua/support/errno.h>
#include <viua/support/print.hh>
#include <viua/support/tty.h>
#include <viua/vm/elf.h>

auto show_symbol_table(
    viua::vm::elf::Loaded_elf const& elf,
    std::string const section_name,
    std::string const strtab_section_name) -> void
{
    auto const frag = elf.find_fragment(section_name);
    if (not frag.has_value()) {
        return;
    }

    auto const& symtab              = frag->get();
    auto const& sh [[maybe_unused]] = symtab.section_header;
    auto const& data                = symtab.data;

    auto const entries = data.size() / sizeof(Elf64_Sym);
    std::println(
        "Symbol table '{}' contains {} entries:", section_name, entries);
    std::println(
        "  Num: Value            Size Type   Bind   Vis       Ndx Name");
    for (auto i = size_t{ 0 }; i < entries; ++i) {
        auto const offset = (i * sizeof(Elf64_Sym));
        auto sym          = Elf64_Sym{};
        memcpy(&sym, data.data() + offset, sizeof(Elf64_Sym));

        auto const type_human_readable = viua::st_type_to_string(sym.st_info);
        auto const bind_human_readable = viua::st_bind_to_string(sym.st_info);
        auto const vis_human_readable =
            viua::st_visibility_to_string(sym.st_other);

        std::println("  {:3d}: {:016x} {:4d} {:6} {:6} {:9} {:>3} {}",
                     i,
                     sym.st_value,
                     sym.st_size,
                     type_human_readable,
                     bind_human_readable,
                     vis_human_readable,
                     viua::st_shndx_to_string(sym.st_shndx),
                     elf.strtab_of(strtab_section_name).view_at(sym.st_name));
    }
}

auto to_hex_byte_string(
    std::string_view const sv) -> std::string
{
    if (sv.empty()) {
        return "";
    }

    return std::accumulate(
        sv.begin() + 1,
        sv.end(),
        std::format("{:02x}", sv[0]),
        [](std::string acc, char const each) -> std::string
        { return std::move(acc) + " " + std::format("{:02x}", each); });
}

auto main(
    int argc,
    char* argv[]) -> int
{
    using viua::support::tty::ATTR_RESET;
    using viua::support::tty::COLOR_FG_CYAN;
    using viua::support::tty::COLOR_FG_ORANGE_RED_1;
    using viua::support::tty::COLOR_FG_RED;
    using viua::support::tty::COLOR_FG_RED_1;
    using viua::support::tty::COLOR_FG_WHITE;
    using viua::support::tty::send_escape_seq;
    constexpr auto esc = send_escape_seq;

    using viua::libexec::Args;
    auto const args = viua::libexec::args_or_exit(
        "readelf",
        argc,
        argv,
        {
            VIUA_TOOL_COMMON_OPTIONS,
            { { "a", { "all" } }, Args::Kind::Switch },
            { { "h", { "file-header" } }, Args::Kind::Switch },
            { { "l", { "program-headers", "segments" } }, Args::Kind::Switch },
            { { "S", { "section-headers", "sections" } }, Args::Kind::Switch },
            { { "g", { "section-groups" } }, Args::Kind::Switch },
            { { "t", { "section-details" } }, Args::Kind::Switch },
            { { "e", { "headers" } }, Args::Kind::Switch },
            { { "s", { "symbols", "syms" } }, Args::Kind::Switch },
            { { "", { "dynamic-symbols", "dyn-syms" } }, Args::Kind::Switch },
            { { "", { "type" } }, Args::Kind::Switch },
        });
    if (args.args.empty()) {
        viua::support::errorln("no path to load");
        return 1;
    }

    auto const show_all         = args.get<bool>("all");
    auto const show_headers     = show_all or args.get<bool>("headers");
    auto const show_headers_elf = show_headers or args.get<bool>("file-header");
    auto const show_headers_sh =
        show_headers or args.get<bool>("section-headers");
    auto const show_symbols = show_all or args.get<bool>("symbols");
    auto const show_symbols_dynamic =
        show_symbols or args.get<bool>("dynamic-symbols");

    auto const elf_path = std::filesystem::path{ args.args.back() };
    if (not std::filesystem::exists(elf_path)) {
        viua::support::errorln("file does not exist: {}{}{}",
                               esc(2, COLOR_FG_WHITE),
                               elf_path.string(),
                               esc(2, ATTR_RESET));
        return 1;
    }

    auto const elf_fd = open(elf_path.c_str(), O_RDONLY);
    if (elf_fd == -1) {
        auto const saved_errno = errno;
        viua::support::errorln(elf_path,
                               "{}: {}",
                               viua::support::errno_name(saved_errno),
                               viua::support::errno_desc(saved_errno));
        return 1;
    }

    using viua::arch::elf::VIUA_MAGIC;
    using viua::vm::elf::Loaded_elf;
    auto const elf = [elf_fd, &elf_path]()
    {
        try {
            return Loaded_elf::load(elf_fd);
        } catch (std::runtime_error const& e) {
            viua::support::errorln(elf_path, "not a Viua ELF");
            exit(2);
        }
    }();

    if (show_headers_elf) {
        std::println("ELF header:");

        auto const ident_view =
            std::string_view{ reinterpret_cast<char const*>(elf.header.e_ident),
                              sizeof(elf.header.e_ident) };
        auto const magic_human_readable = to_hex_byte_string(ident_view);
        std::println("  Magic:                      {}", magic_human_readable);

        std::println("  Class:                      {}",
                     viua::elf_class_to_string(elf.header.e_ident));
        std::println("  Data:                       {}",
                     viua::elf_data_to_string(elf.header.e_ident));
        std::println("  OS/ABI:                     {}",
                     viua::elf_osabi_to_string(elf.header.e_ident));
        std::println("  ABI version:                {}",
                     viua::elf_abiversion_to_string(elf.header.e_ident));

        std::println("  Type:                       {}",
                     viua::elf_type_to_string(elf.header.e_type));
        auto const is_exec = (elf.header.e_type == ET_EXEC);
        if (is_exec) {
            auto const ep = elf.entry_point();
            auto const ep_human_readable =
                ep.has_value()
                    ? std::format(
                          "0x{:x}  [.text+0x{:x}]", elf.header.e_entry, *ep)
                    : "not found";
            std::println("  Entry:                      {}", ep_human_readable);
        }

        std::println("  Machine:                    {}",
                     viua::elf_machine_to_string(elf.header.e_machine));
        std::println("  Version:                    {}", elf.header.e_version);

        std::println(
            "  Flags:                      0x{:08x}", elf.header.e_flags);

        std::println("  Section headers, offset of: 0x{:x} (bytes into file)",
                     elf.header.e_shoff);
        std::println("  Section headers, number of: {}", elf.header.e_shnum);
        std::println(
            "  Section headers, size of:   {} (bytes)", elf.header.e_shentsize);

        std::println("  Program headers, offset of: 0x{:x} (bytes into file)",
                     elf.header.e_phoff);
        std::println("  Program headers, number of: {}", elf.header.e_phnum);
        std::println(
            "  Program headers, size of:   {} (bytes)", elf.header.e_phentsize);

        std::println("  Section index of .shstrtab: {}", elf.header.e_shstrndx);
    }

    if (show_headers_sh) {
        std::println("Fragments:");

        auto section_header_index = size_t{ 0 };
        for (auto const& [name, each] : elf.fragments) {
            auto const& sh [[maybe_unused]] = each.section_header;
            auto const& ph [[maybe_unused]] = each.program_header;

            std::println("  [{:2d}] {:20} {} in {}",
                         section_header_index++,
                         name,
                         viua::sh_type_to_string(sh.sh_type),
                         viua::p_type_to_string(ph->p_type));

            std::println("       Offset       {:016x} ({} bytes)",
                         sh.sh_offset,
                         sh.sh_offset);
            std::println("       Size         0x{:x} ({} bytes)",
                         sh.sh_size,
                         sh.sh_size);

            if (ph and (ph->p_type == PT_LOAD)) {
                std::println("       Memory size  0x{:x} ({} bytes)",
                             ph->p_memsz,
                             ph->p_memsz);
            }

            if (name == ".interp") {
                auto const interp = std::string_view{
                    reinterpret_cast<char const*>(each.data.data()),
                    each.data.size()
                };
                std::println("         [Interpreter: {}]", interp);
            }
            if (name == ".viua.magic") {
                if (not ph.has_value()) {
                    viua::support::errorln(
                        elf_path, "no program header with the magic value");
                } else {
                    auto const got_magic = std::string_view{
                        reinterpret_cast<char const*>(&ph->p_paddr),
                        sizeof(ph->p_paddr)
                    };
                    auto const valid = (got_magic == VIUA_MAGIC);

                    std::println("         [Magic: {}{}]",
                                 to_hex_byte_string(got_magic),
                                 (valid ? "" : " (invalid)"));

                    if (not valid) {
                        std::println("         [Wants: {}]",
                                     to_hex_byte_string(VIUA_MAGIC));
                    }
                }
            }
        }
    }

    if (show_symbols) {
        show_symbol_table(elf, ".symtab", ".strtab");
    }
    if (show_symbols_dynamic) {
        show_symbol_table(elf, ".dynsym", ".dynstr");
    }

    return 0;
}
