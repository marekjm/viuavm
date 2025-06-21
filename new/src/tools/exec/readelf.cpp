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
        std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                  << ": file does not exist: " << esc(2, COLOR_FG_WHITE)
                  << elf_path.string() << esc(2, ATTR_RESET) << "\n";
        return 1;
    }

    auto const elf_fd = open(elf_path.c_str(), O_RDONLY);
    if (elf_fd == -1) {
        auto const saved_errno = errno;
        auto const errname     = viua::support::errno_name(saved_errno);
        auto const errdesc     = viua::support::errno_desc(saved_errno);

        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.string()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET) << ": " << errname << ": " << errdesc
                  << "\n";
        return 1;
    }

    using viua::vm::elf::Loaded_elf;
    using viua::vm::elf::VIUA_MAGIC;
    auto const elf = [elf_fd, &elf_path]()
    {
        try {
            return Loaded_elf::load(elf_fd);
        } catch (std::runtime_error const& e) {
            viua::support::errorln("not a Viua ELF: {}{}{}",
                                   esc(2, COLOR_FG_WHITE),
                                   elf_path.string(),
                                   esc(2, ATTR_RESET));
            exit(2);
        }
    }();

    if (show_headers_elf) {
        std::println("ELF header:");

        auto const magic_human_readable = std::accumulate(
            std::begin(elf.header.e_ident) + 1,
            std::end(elf.header.e_ident),
            std::format("{:x}", elf.header.e_ident[0]),
            [](std::string acc, uint8_t const each) -> std::string
            { return std::move(acc) + " " + std::format("{:x}", each); });
        std::println("  Magic: {}", magic_human_readable);

        auto const is_exec = (elf.header.e_type == ET_EXEC);
        std::println("  Type:  {}",
                     (is_exec ? "EXEC (Executable)" : "REL (Relocatable)"));
        if (is_exec) {
            auto const ep = elf.entry_point();
            auto const ep_human_readable =
                ep.has_value()
                    ? std::format(
                          "0x{:016x}  [.text+0x{:x}]", elf.header.e_entry, *ep)
                    : "not found";
            std::println("  Entry: {}", ep_human_readable);
        }
    }

    if (show_headers_sh) {
        std::cout << "Fragments:\n";

        auto const index_width    = std::to_string(elf.fragments.size()).size();
        auto section_header_index = size_t{ 0 };
        for (auto const& [name, each] : elf.fragments) {
            auto const& sh [[maybe_unused]] = each.section_header;
            auto const& ph [[maybe_unused]] = each.program_header;

            {
                constexpr auto NAME_WIDTH = 20;
                std::cout << "  [" << std::setw(index_width)
                          << section_header_index++ << "] ";
                std::cout << name
                          << std::string((NAME_WIDTH - name.size()), ' ');
                std::cout << viua::sh_type_to_string(sh.sh_type);
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
                          << std::setw(sizeof(sh.sh_offset))
                          << std::setfill('0') << std::hex << sh.sh_offset
                          << std::dec << " (" << sh.sh_offset << " bytes)"
                          << "\n";

                std::cout << INDENT << "Size         "
                          << std::setw(sizeof(sh.sh_size)) << std::setfill('0')
                          << std::hex << sh.sh_size << std::dec << " ("
                          << sh.sh_size << " bytes)"
                          << "\n";

                if (ph and (ph->p_type == PT_LOAD)) {
                    std::cout << INDENT << "Memory size  "
                              << std::setw(sizeof(ph->p_memsz))
                              << std::setfill('0') << std::hex << ph->p_memsz
                              << std::dec << " (" << ph->p_memsz << " bytes)"
                              << "\n";
                }
            }

            if (name == ".interp") {
                std::cout << INDENT << "  [Interpreter: " << each.data.data()
                          << "]\n";
            }
            if (name == ".viua.magic") {
                if (not ph.has_value()) {
                    std::cerr << "No program header for magic value!\n";
                }

                std::cout << INDENT << "  [Magic:";
                std::cout << std::hex;
                auto const got_magic = std::string_view{
                    reinterpret_cast<char const*>(&ph->p_paddr),
                    sizeof(ph->p_paddr)
                };
                for (auto const c : got_magic) {
                    std::cout << ' ' << std::setw(2) << std::setfill('0')
                              << static_cast<int>(c);
                }
                std::cout << std::dec;

                auto const valid = (got_magic == VIUA_MAGIC);
                std::cout << (valid ? "" : " (invalid)") << "]\n";
                if (not valid) {
                    std::cout << INDENT << "  [Wants:";
                    std::cout << std::hex;
                    for (auto const c : VIUA_MAGIC) {
                        std::cout << ' ' << std::setw(2) << std::setfill('0')
                                  << static_cast<int>(c);
                    }
                    std::cout << std::dec;
                    std::cout << "]\n";
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
