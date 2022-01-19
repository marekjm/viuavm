/*
 *  Copyright (C) 2021-2022 Marek Marecki
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
#include <liburing.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <viua/arch/arch.h>
#include <viua/arch/ins.h>
#include <viua/arch/ops.h>
#include <viua/support/fdstream.h>
#include <viua/support/string.h>
#include <viua/support/tty.h>
#include <viua/vm/core.h>
#include <viua/vm/elf.h>
#include <viua/vm/ins.h>
#include <viua/vm/types.h>


constexpr auto VIUA_TRACE_CYCLES = true;
constexpr auto VIUA_SLOW_CYCLES  = false;

namespace viua {
auto TRACE_STREAM = viua::support::fdstream{2};
}


namespace {
auto execute(viua::vm::Stack& stack, viua::arch::instruction_type const* const ip)
    -> viua::arch::instruction_type const*
{
    auto const raw = *ip;

    auto const opcode = static_cast<viua::arch::opcode_type>(
        raw & viua::arch::ops::OPCODE_MASK);
    auto const format = static_cast<viua::arch::ops::FORMAT>(
        opcode & viua::arch::ops::FORMAT_MASK);


    switch (format) {
        using viua::vm::ins::execute;
        using namespace viua::arch::ins;
        using enum viua::arch::ops::FORMAT;
    case T:
    {
        auto instruction = viua::arch::ops::T::decode(raw);
        if constexpr (VIUA_TRACE_CYCLES) {
            viua::TRACE_STREAM << "    " << instruction.to_string()
                               << viua::TRACE_STREAM.endl;
        }

        using viua::arch::ops::OPCODE_T;
        switch (static_cast<OPCODE_T>(opcode)) {
        case OPCODE_T::ADD:
            execute(ADD{instruction}, stack, ip);
            break;
        case OPCODE_T::SUB:
            execute(SUB{instruction}, stack, ip);
            break;
        case OPCODE_T::MUL:
            execute(MUL{instruction}, stack, ip);
            break;
        case OPCODE_T::DIV:
            execute(DIV{instruction}, stack, ip);
            break;
        case OPCODE_T::MOD:
            execute(MOD{instruction}, stack, ip);
            break;
        case OPCODE_T::BITSHL:
            execute(BITSHL{instruction}, stack, ip);
            break;
        case OPCODE_T::BITSHR:
            execute(BITSHR{instruction}, stack, ip);
            break;
        case OPCODE_T::BITASHR:
            execute(BITASHR{instruction}, stack, ip);
            break;
        case OPCODE_T::BITROL:
            execute(BITROL{instruction}, stack, ip);
            break;
        case OPCODE_T::BITROR:
            execute(BITROR{instruction}, stack, ip);
            break;
        case OPCODE_T::BITAND:
            execute(BITAND{instruction}, stack, ip);
            break;
        case OPCODE_T::BITOR:
            execute(BITOR{instruction}, stack, ip);
            break;
        case OPCODE_T::BITXOR:
            execute(BITXOR{instruction}, stack, ip);
            break;
        case OPCODE_T::EQ:
            execute(EQ{instruction}, stack, ip);
            break;
        case OPCODE_T::GT:
            execute(GT{instruction}, stack, ip);
            break;
        case OPCODE_T::LT:
            execute(LT{instruction}, stack, ip);
            break;
        case OPCODE_T::CMP:
            execute(CMP{instruction}, stack, ip);
            break;
        case OPCODE_T::AND:
            execute(AND{instruction}, stack, ip);
            break;
        case OPCODE_T::OR:
            execute(OR{instruction}, stack, ip);
            break;
        case OPCODE_T::BUFFER_AT:
            execute(BUFFER_AT{instruction}, stack, ip);
            break;
        case OPCODE_T::BUFFER_POP:
            execute(BUFFER_POP{instruction}, stack, ip);
            break;
        case OPCODE_T::STRUCT_AT:
            execute(STRUCT_AT{instruction}, stack, ip);
            break;
        case OPCODE_T::STRUCT_INSERT:
            execute(STRUCT_INSERT{instruction}, stack, ip);
            break;
        case OPCODE_T::STRUCT_REMOVE:
            execute(STRUCT_REMOVE{instruction}, stack, ip);
            break;
        case OPCODE_T::IO_SUBMIT:
            execute(IO_SUBMIT{instruction}, stack, ip);
            break;
        case OPCODE_T::IO_WAIT:
            execute(IO_WAIT{instruction}, stack, ip);
            break;
        case OPCODE_T::IO_SHUTDOWN:
            execute(IO_SHUTDOWN{instruction}, stack, ip);
            break;
        case OPCODE_T::IO_CTL:
            execute(IO_CTL{instruction}, stack, ip);
            break;
        }
        break;
    }
    case S:
    {
        auto instruction = viua::arch::ops::S::decode(raw);
        if constexpr (VIUA_TRACE_CYCLES) {
            viua::TRACE_STREAM << "    " << instruction.to_string()
                               << viua::TRACE_STREAM.endl;
        }

        using viua::arch::ops::OPCODE_S;
        switch (static_cast<OPCODE_S>(opcode)) {
        case OPCODE_S::FRAME:
            execute(FRAME{instruction}, stack, ip);
            break;
        case OPCODE_S::RETURN:
            return execute(RETURN{instruction}, stack, ip);
        case OPCODE_S::ATOM:
            execute(ATOM{instruction}, stack, ip);
            break;
        case OPCODE_S::STRING:
            execute(STRING{instruction}, stack, ip);
            break;
        case OPCODE_S::FLOAT:
            execute(FLOAT{instruction}, stack, ip);
            break;
        case OPCODE_S::DOUBLE:
            execute(DOUBLE{instruction}, stack, ip);
            break;
        case OPCODE_S::STRUCT:
            execute(STRUCT{instruction}, stack, ip);
            break;
        case OPCODE_S::BUFFER:
            execute(BUFFER{instruction}, stack, ip);
            break;
        }
        break;
    }
    case E:
    {
        auto instruction = viua::arch::ops::E::decode(raw);
        if constexpr (VIUA_TRACE_CYCLES) {
            viua::TRACE_STREAM << "    " << instruction.to_string()
                               << viua::TRACE_STREAM.endl;
        }

        using viua::arch::ops::OPCODE_E;
        switch (static_cast<OPCODE_E>(opcode)) {
        case OPCODE_E::LUI:
            execute(LUI{instruction}, stack, ip);
            break;
        case OPCODE_E::LUIU:
            execute(LUIU{instruction}, stack, ip);
            break;
        }
        break;
    }
    case R:
    {
        auto instruction = viua::arch::ops::R::decode(raw);
        if constexpr (VIUA_TRACE_CYCLES) {
            viua::TRACE_STREAM << "    " << instruction.to_string()
                               << viua::TRACE_STREAM.endl;
        }

        using viua::arch::ops::OPCODE_R;
        switch (static_cast<OPCODE_R>(opcode)) {
        case OPCODE_R::ADDI:
            execute(ADDI{instruction}, stack, ip);
            break;
        case OPCODE_R::ADDIU:
            execute(ADDIU{instruction}, stack, ip);
            break;
        case OPCODE_R::SUBI:
            execute(SUBI{instruction}, stack, ip);
            break;
        case OPCODE_R::SUBIU:
            execute(SUBIU{instruction}, stack, ip);
            break;
        case OPCODE_R::MULI:
            execute(MULI{instruction}, stack, ip);
            break;
        case OPCODE_R::MULIU:
            execute(MULIU{instruction}, stack, ip);
            break;
        case OPCODE_R::DIVI:
            execute(DIVI{instruction}, stack, ip);
            break;
        case OPCODE_R::DIVIU:
            execute(DIVIU{instruction}, stack, ip);
            break;
        }
        break;
    }
    case N:
    {
        if constexpr (VIUA_TRACE_CYCLES) {
            viua::TRACE_STREAM << "    " << viua::arch::ops::to_string(opcode)
                               << viua::TRACE_STREAM.endl;
        }

        using viua::arch::ops::OPCODE_N;
        switch (static_cast<OPCODE_N>(opcode)) {
        case OPCODE_N::NOOP:
            break;
        case OPCODE_N::HALT:
            return nullptr;
        case OPCODE_N::EBREAK:
            execute(EBREAK{viua::arch::ops::N::decode(raw)}, stack, ip);
            viua::TRACE_STREAM << "    #ebreak" << viua::TRACE_STREAM.endl;
            break;
        }
        break;
    }
    case D:
    {
        auto instruction = viua::arch::ops::D::decode(raw);
        if constexpr (VIUA_TRACE_CYCLES) {
            viua::TRACE_STREAM << "    " << instruction.to_string()
                               << viua::TRACE_STREAM.endl;
        }

        using viua::arch::ops::OPCODE_D;
        switch (static_cast<OPCODE_D>(opcode)) {
        case OPCODE_D::CALL:
            /*
             * Call is a special instruction. It transfers the IP to a
             * semi-random location, instead of just increasing it to the next
             * unit.
             *
             * This is why we return here, and not use the default behaviour for
             * most of the other instructions.
             */
            return execute(CALL{instruction}, stack, ip);
        case OPCODE_D::BITNOT:
            execute(BITNOT{instruction}, stack, ip);
            break;
        case OPCODE_D::NOT:
            execute(NOT{instruction}, stack, ip);
            break;
        case OPCODE_D::COPY:
            execute(COPY{instruction}, stack, ip);
            break;
        case OPCODE_D::MOVE:
            execute(MOVE{instruction}, stack, ip);
            break;
        case OPCODE_D::SWAP:
            execute(SWAP{instruction}, stack, ip);
            break;
        case OPCODE_D::BUFFER_PUSH:
            execute(BUFFER_PUSH{instruction}, stack, ip);
            break;
        case OPCODE_D::BUFFER_SIZE:
            execute(BUFFER_SIZE{instruction}, stack, ip);
            break;
        case OPCODE_D::REF:
            execute(REF{instruction}, stack, ip);
            break;
        case OPCODE_D::IF:
            /*
             * If is a special instruction. It transfers IP to a semi-random
             * location instead of just increasing it to the next unit. This is
             * why the return is used instead of break.
             */
            return execute(IF{instruction}, stack, ip);
        case OPCODE_D::IO_PEEK:
            execute(IO_PEEK{instruction}, stack, ip);
            break;
        }
        break;
    }
    case F:
        std::cerr << "unimplemented instruction: "
                  << viua::arch::ops::to_string(opcode) << "\n";
        return nullptr;
    }

    return (ip + 1);
}

auto run_instruction(viua::vm::Stack& stack, viua::arch::instruction_type const* ip)
    -> viua::arch::instruction_type const*
{
    auto instruction = viua::arch::instruction_type{};
    do {
        instruction = *ip;
        ip          = execute(stack, ip);
    } while ((ip != nullptr) and (instruction & viua::arch::ops::GREEDY));

    return ip;
}

auto run(viua::vm::Stack& stack, viua::arch::instruction_type const* ip) -> void
{
    constexpr auto PREEMPTION_THRESHOLD = size_t{2};

    while (stack.module.ip_in_valid_range(ip)) {
        if constexpr (VIUA_TRACE_CYCLES) {
            viua::TRACE_STREAM
                << "cycle at " << stack.module.elf_path.native()
                << "[.text+0x" << std::hex << std::setw(8) << std::setfill('0')
                << ((ip - stack.module.ip_base)
                    * sizeof(viua::arch::instruction_type))
                << std::dec << ']' << viua::TRACE_STREAM.endl;
        }

        auto const ip_ok = [&stack, &ip]() -> bool {
            return stack.module.ip_in_valid_range(ip);
        };
        for (auto i = size_t{0}; i < PREEMPTION_THRESHOLD and ip_ok(); ++i) {
            /*
             * This is needed to detect greedy bundles and adjust preemption
             * counter appropriately. If a greedy bundle contains more
             * instructions than the preemption threshold allows the process
             * will be suspended immediately.
             */
            auto const greedy    = (*ip & viua::arch::ops::GREEDY);
            auto const bundle_ip = ip;

            ip = run_instruction(stack, ip);

            /*
             * If the instruction was a greedy bundle instead of a single
             * one, the preemption counter has to be adjusted. It may be the
             * case that the bundle already hit the preemption threshold.
             */
            if (greedy and ip_ok()) {
                i += (ip - bundle_ip) - 1;
            }
        }

        if (stack.frames.empty()) {
            // std::cerr << "exited\n";
            break;
        }
        if (not ip_ok()) {
            // std::cerr << "halted\n";
            break;
        }

        if constexpr (VIUA_SLOW_CYCLES) {
            /*
             * FIXME Limit the amount of instructions executed per second
             * for debugging purposes. Once everything works as it should,
             * remove this code.
             */
            using namespace std::literals;
            std::this_thread::sleep_for(160ms);
        }
    }

    if (not stack.module.ip_in_valid_range(ip)) {
        std::cerr << "[vm] ip " << std::hex << std::setw(8) << std::setfill('0')
                  << ((ip - stack.module.ip_base)
                      * sizeof(viua::arch::instruction_type))
                  << std::dec << " outside of valid range\n";
    }
}
}  // namespace

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
                  << ": no executable to run\n";
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

    /*
     * Do not assume that the path given by the user points to a file that
     * exists. Typos are a thing. And let's check if the file really is a
     * regular file - trying to execute directories or device files does not
     * make much sense.
     */
    auto const elf_path = std::filesystem::path{args.back()};
    if (not std::filesystem::exists(elf_path)) {
        std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                  << ": file does not exist: " << esc(2, COLOR_FG_WHITE)
                  << elf_path.native() << esc(2, ATTR_RESET) << "\n";
        return 1;
    }
    {
        struct stat statbuf {
        };
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

    if (auto const f = main_module.find_fragment(".rodata");
        not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET) << ": no strings fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .rodata section found\n";
        return 1;
    }
    if (auto const f = main_module.find_fragment(".viua.fns");
        not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET)
                  << ": no function table fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .viua.fns section found\n";
        return 1;
    }
    if (auto const f = main_module.find_fragment(".text"); not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET) << ": no text fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .text section found\n";
        return 1;
    }

    auto entry_addr = size_t{0};
    if (auto const ep = main_module.entry_point(); ep.has_value()) {
        entry_addr = (*ep / sizeof(viua::arch::instruction_type));
    } else {
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET) << ": no entry point defined\n";
        return 1;
    }

    auto mod = viua::vm::Module{elf_path, main_module};

    auto stack = viua::vm::Stack{mod};
    stack.push(256, (mod.ip_base + entry_addr), nullptr);

    if constexpr (VIUA_TRACE_CYCLES) {
        if (auto trace_fd = getenv("VIUA_VM_TRACE_FD"); trace_fd) {
            try {
                /*
                 * Assume an file descriptor opened for writing was received.
                 */
                viua::TRACE_STREAM =
                    viua::support::fdstream{std::stoi(trace_fd)};
            } catch (std::invalid_argument const&) {
                /*
                 * Otherwise, treat the thing received as a filename and open it
                 * for writing.
                 */
                viua::TRACE_STREAM = viua::support::fdstream{
                    open(trace_fd, O_WRONLY | O_CLOEXEC)};
            }
        }
    }

    if constexpr (true) {
        run(stack, stack.back().entry_address);
    } else {
        try {
            run(stack, stack.back().entry_address);
        } catch (viua::vm::abort_execution const& e) {
            std::cerr << "Aborted execution: " << e.what() << "\n";
            return 1;
        }
    }

    return 0;
}
