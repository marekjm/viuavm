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

#include <viua/support/tty.h>
#include <viua/vm/elf.h>

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

    auto const args = std::vector<std::string>{(argv + 1), (argv + argc)};
    if (args.empty()) {
        std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                  << ": not path to load\n";
        return 1;
    }

    auto verbosity_level = 0;
    auto show_version    = false;

    for (auto i = decltype(args)::size_type{}; i < args.size(); ++i) {
        auto const& each = args.at(i);
        if (each == "--") {
            // explicit separator of options and operands
            break;
        }
        /*
         * Common options.
         */
        else if (each == "-v" or each == "--verbose") {
            ++verbosity_level;
        } else if (each == "--version") {
            show_version = true;
        } else if (each.front() == '-') {
            // unknown option
        } else {
            // input files start here
            break;
        }
    }

    if (show_version) {
        if (verbosity_level) {
            std::cout << "Viua VM ";
        }
        std::cout << (verbosity_level ? VIUAVM_VERSION_FULL : VIUAVM_VERSION)
                  << "\n";
        return 0;
    }

    auto const elf_path = std::filesystem::path{args.back()};
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

    using viua::vm::elf::Loaded_elf;
    using viua::vm::elf::VIUA_MAGIC;
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
