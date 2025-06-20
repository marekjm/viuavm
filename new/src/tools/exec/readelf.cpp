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
        });
    if (args.args.empty()) {
        viua::support::errorln("no path to load");
        return 1;
    }

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
    auto const elf = Loaded_elf::load(elf_fd);
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
            std::cout << name << std::string((NAME_WIDTH - name.size()), ' ');
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
            for (auto const c : each.data) {
                std::cout << ' ' << std::setw(2) << std::setfill('0')
                          << static_cast<int>(c);
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
        std::cout << "  " << elf.name_function_at(*ep) << "\n";
    } else {
        std::cout << "not found\n";
    }
    std::println();

    show_symbol_table(elf, ".symtab", ".strtab");
    show_symbol_table(elf, ".dynsym", ".dynstr");

    return 0;
}
