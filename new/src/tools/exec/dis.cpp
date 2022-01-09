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


#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

#include <iostream>
#include <filesystem>

#include <viua/arch/ops.h>
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

    if (argc == 1) {
        std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                  << ": no file to disassemble\n";
        return 1;
    }

    /*
     * Do not assume that the path given by the user points to a file that
     * exists. Typos are a thing. And let's check if the file really is a
     * regular file - trying to execute directories or device files does not
     * make much sense.
     */
    auto const elf_path = std::filesystem::path{argv[1]};
    if (not std::filesystem::exists(elf_path)) {
        std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                  << ": file does not exist: " << esc(2, COLOR_FG_WHITE)
                  << elf_path.native() << esc(2, ATTR_RESET) << "\n";
        return 1;
    }
    {
        struct stat statbuf {};
        if (stat(elf_path.c_str(), &statbuf) == -1) {
            auto const saved_errno = errno;
            auto const errname     = strerrorname_np(saved_errno);
            auto const errdesc     = strerrordesc_np(saved_errno);

            std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                      << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                      << esc(2, ATTR_RESET);
            if (errname) {
                std::cerr << ": " << errname;
            }
            std::cerr << ": " << (errdesc ? errdesc : "unknown error") << "\n";
            return 1;
        }
        if ((statbuf.st_mode & S_IFMT) != S_IFREG) {
            std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                      << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                      << esc(2, ATTR_RESET);
            std::cerr << ": not a regular file\n";
            return 1;
        }
    }

    /*
     * Even if the path exists and is a regular file we should check if it was
     * opened correctly.
     */
    auto const elf_fd = open(elf_path.c_str(), O_RDONLY);
    if (elf_fd == -1) {
        auto const saved_errno = errno;
        auto const errname     = strerrorname_np(saved_errno);
        auto const errdesc     = strerrordesc_np(saved_errno);

        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET);
        if (errname) {
            std::cerr << ": " << errname;
        }
        std::cerr << ": " << (errdesc ? errdesc : "unknown error") << "\n";
        return 1;
    }

    using Module           = viua::vm::elf::Loaded_elf;
    auto const main_module = Module::load(elf_fd);

    if (auto const f = main_module.find_fragment(".rodata"); not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET) << ": no strings fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .rodata section found\n";
        return 1;
    }
    if (auto const f = main_module.find_fragment(".viua.fns"); not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET)
                  << ": no function table fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .viua.fns section found\n";
        return 1;
    }

    auto text = std::vector<viua::arch::instruction_type>{};
    if (auto const f = main_module.find_fragment(".text"); not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET) << ": no text fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .text section found\n";
        return 1;
    } else {
        text = main_module.make_text_from(f->get().data);
    }

    auto entry_addr = size_t{0};
    if (auto const ep = main_module.entry_point(); ep.has_value()) {
        entry_addr = *ep;
    } else {
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET) << ": no entry point defined\n";
        return 1;
    }

    auto const ft = main_module.function_table();
    auto ordered_fns = std::vector<std::tuple<std::string, size_t, size_t>>{};
    {
        for (auto const& each : ft) {
            ordered_fns.push_back({ each.second.first, each.second.second, 0 });
        }
        std::sort(ordered_fns.begin(), ordered_fns.end(), [](auto const& a, auto const& b)
        {
            return (std::get<1>(a) < std::get<1>(b));
        });
        for (auto i = size_t{1}; i < ordered_fns.size(); ++i) {
            auto const& each = ordered_fns.at(i);
            auto& prev = ordered_fns.at(i - 1);
            std::get<2>(prev) = (std::get<1>(each) - std::get<1>(prev)) / sizeof(viua::arch::instruction_type);
        }
        std::get<2>(ordered_fns.back()) = ((text.size() * sizeof(viua::arch::instruction_type)) - std::get<1>(ordered_fns.back())) / sizeof(viua::arch::instruction_type);
    }

    auto ef = main_module.name_function_at(entry_addr);
    for (auto const& [ name, addr, size ] : ordered_fns) {
        /*
         * Let's emit a comment containing the span of the function. This is
         * would be useful if you wanted to map the disassembled span to bytes
         * inside the bytecode segment.
         */
        std::cout << "; [.text+0x" << std::setw(16) << std::setfill('0') << addr << "] to "
                  << "[.text+0x" <<  std::setw(16) << std::setfill('0')
                  << (addr + (size * sizeof(viua::arch::instruction_type)))
                  << "] (" << size << " instruction" << ((size > 1) ? "s" : "")
                  << ")\n";

        /*
         * Then, the name. Marking the entry point is necessary to correctly
         * recreate the behaviour of the program.
         */
        std::cout << ".function: "
            << ((ef.first == name) ? "[[entry_point]] " : "")
            << name << "\n";

        for (auto i = (addr / sizeof(viua::arch::instruction_type)); i <= size; ++i) {
            auto const ip = text.at(i);

            std::cout << "    ; ";
            std::cout << std::setw(16) << std::setfill('0') << std::hex
                << ip << "\n";

            auto const opcode = static_cast<viua::arch::opcode_type>(
                ip & viua::arch::ops::OPCODE_MASK);
            auto const format = static_cast<viua::arch::ops::FORMAT>(
                opcode & viua::arch::ops::FORMAT_MASK);

            std::cout << "    ";
            switch (format) {
                using enum viua::arch::ops::FORMAT;
            case N:
                std::cout << viua::arch::ops::to_string(opcode);
                break;
            case T:
                std::cout << viua::arch::ops::T::decode(ip).to_string();
                break;
            case D:
                std::cout << viua::arch::ops::D::decode(ip).to_string();
                break;
            case S:
                std::cout << viua::arch::ops::S::decode(ip).to_string();
                break;
            case F:
                std::cout << viua::arch::ops::F::decode(ip).to_string();
                break;
            case E:
                std::cout << viua::arch::ops::E::decode(ip).to_string();
                break;
            case R:
                std::cout << viua::arch::ops::R::decode(ip).to_string();
                break;
            default:
                std::cout << "; " << std::string(16, '^') << " invalid instruction";
                break;
            }
            std::cout << "\n";
        }

        std::cout << ".end\n\n";
    }

    return 0;
}
