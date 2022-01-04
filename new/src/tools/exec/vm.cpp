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
#include <unistd.h>

#include <chrono>
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
#include <viua/vm/core.h>
#include <viua/vm/ins.h>
#include <viua/vm/types.h>


constexpr auto VIUA_TRACE_CYCLES = true;
constexpr auto VIUA_SLOW_CYCLES  = false;

namespace viua {
auto TRACE_STREAM = viua::support::fdstream{2};
}


using viua::vm::Env;
using viua::vm::Stack;


auto Env::function_at(size_t const offset) const
    -> std::pair<std::string, size_t>
{
    auto sz = uint64_t{};
    memcpy(&sz, (functions_table.data() + offset - sizeof(sz)), sizeof(sz));
    sz = le64toh(sz);

    auto name = std::string{
        reinterpret_cast<char const*>(functions_table.data()) + offset, sz};

    auto addr = uint64_t{};
    memcpy(&addr, (functions_table.data() + offset + sz), sizeof(addr));
    addr = le64toh(addr);

    return {name, addr};
}

namespace {
auto execute(Stack& stack, viua::arch::instruction_type const* const ip)
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

auto run_instruction(Stack& stack, viua::arch::instruction_type const* ip)
    -> viua::arch::instruction_type const*
{
    auto instruction = viua::arch::instruction_type{};
    do {
        instruction = *ip;
        ip          = execute(stack, ip);
    } while ((ip != nullptr) and (instruction & viua::arch::ops::GREEDY));

    return ip;
}

auto run(Stack& stack,
         viua::arch::instruction_type const* ip,
         std::tuple<std::string_view const,
                    viua::arch::instruction_type const*,
                    viua::arch::instruction_type const*> ip_range) -> void
{
    auto const [module, ip_begin, ip_end] = ip_range;

    constexpr auto PREEMPTION_THRESHOLD = size_t{2};

    while ((ip < ip_end) and (ip >= ip_begin)) {
        if constexpr (VIUA_TRACE_CYCLES) {
            viua::TRACE_STREAM
                << "cycle at " << module << "+0x" << std::hex << std::setw(8)
                << std::setfill('0')
                << ((ip - ip_begin) * sizeof(viua::arch::instruction_type))
                << std::dec << viua::TRACE_STREAM.endl;
        }

        for (auto i = size_t{0}; i < PREEMPTION_THRESHOLD and ip != ip_end;
             ++i) {
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
             * Halting instruction returns nullptr because it does not know
             * where the end of bytecode lies. This is why we have to watch
             * out for null pointer here.
             */
            ip = (ip == nullptr ? ip_end : ip);

            /*
             * If the instruction was a greedy bundle instead of a single
             * one, the preemption counter has to be adjusted. It may be the
             * case that the bundle already hit the preemption threshold.
             */
            if (greedy and ip != ip_end) {
                i += (ip - bundle_ip) - 1;
            }
        }

        if (stack.frames.empty()) {
            // std::cerr << "exited\n";
            break;
        }
        if (ip == ip_end) {
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

    if ((ip > ip_end) or (ip < ip_begin)) {
        std::cerr << "[vm] ip " << std::hex << std::setw(8) << std::setfill('0')
                  << ((ip - ip_begin) * sizeof(viua::arch::instruction_type))
                  << std::dec << " outside of valid range\n";
    }
}
}  // namespace

auto main(int argc, char* argv[]) -> int
{
    /*
     * If invoked with some operands, use the first of them as the
     * binary to load and execute. It most probably will be the sample
     * executable generated by an earlier invokation of the codec
     * testing program.
     */
    auto const executable_path = std::string{(argc > 1) ? argv[1] : "./a.out"};

    auto strings  = std::vector<uint8_t>{};
    auto fn_table = std::vector<uint8_t>{};
    auto text     = std::vector<viua::arch::instruction_type>{};

    auto entry_addr = size_t{0};

    {
        auto const a_out = open(executable_path.c_str(), O_RDONLY);
        if (a_out == -1) {
            close(a_out);
            exit(1);
        }

        Elf64_Ehdr elf_header{};
        read(a_out, &elf_header, sizeof(elf_header));

        Elf64_Phdr program_header{};

        /*
         * We need to skip a few program headers which are just used to make
         * the file a proper ELF as recognised by file(1) and readelf(1).
         */
        read(a_out, &program_header, sizeof(program_header));  // skip magic
                                                               // PT_NULL
        read(a_out, &program_header, sizeof(program_header));  // skip PT_INTERP

        /*
         * Then come the actually useful program headers describing PT_LOAD
         * segments with .text section (containing the instructions we need to
         * run the program), and .rodata (containing the strings representing
         * strings and symbols).
         */
        Elf64_Phdr text_header{};
        read(a_out, &text_header, sizeof(text_header));

        Elf64_Phdr strings_header{};
        read(a_out, &strings_header, sizeof(strings_header));

        Elf64_Phdr fn_header{};
        read(a_out, &fn_header, sizeof(fn_header));

        lseek(a_out, text_header.p_offset, SEEK_SET);
        text.resize(text_header.p_filesz
                    / sizeof(viua::arch::instruction_type));
        read(a_out, text.data(), text_header.p_filesz);

        if (strings_header.p_filesz) {
            lseek(a_out, strings_header.p_offset, SEEK_SET);
            strings.resize(strings_header.p_filesz);
            read(a_out, strings.data(), strings_header.p_filesz);
        }
        if (fn_header.p_filesz) {
            lseek(a_out, fn_header.p_offset, SEEK_SET);
            fn_table.resize(fn_header.p_filesz);
            read(a_out, fn_table.data(), fn_header.p_filesz);
        }

        close(a_out);

        entry_addr = ((elf_header.e_entry - text_header.p_offset)
                      / sizeof(viua::arch::instruction_type));

        std::cerr << "[vm] loaded " << text_header.p_filesz
                  << " byte(s) of .text section from PT_LOAD segment of "
                  << executable_path << "\n";
        std::cerr << "[vm] loaded "
                  << (text_header.p_filesz / sizeof(decltype(text)::value_type))
                  << " instructions\n";
        std::cerr
            << "[vm] loaded " << strings_header.p_filesz
            << " byte(s) of .rodata (strings) section from PT_LOAD segment of "
            << executable_path << "\n";
        std::cerr
            << "[vm] loaded " << fn_header.p_filesz
            << " byte(s) of .rodata (fn table) section from PT_LOAD segment of "
            << executable_path << "\n";
        std::cerr << "[vm] .text address at +0x" << std::hex << std::setw(8)
                  << std::setfill('0') << text_header.p_offset << std::dec
                  << "\n";
        if (elf_header.e_type == ET_EXEC) {
            std::cerr << "[vm] ELF entry address at +0x" << std::hex << std::setw(8)
                      << std::setfill('0') << elf_header.e_entry << std::dec
                      << "\n";
            std::cerr << "[vm] entry address at [.text]+0x" << std::hex
                      << std::setw(8) << std::setfill('0')
                      << (entry_addr * sizeof(viua::arch::instruction_type))
                      << std::dec << "\n";
        } else {
            std::cerr << "[vm] not an executable, aborting execution\n";
            return 1;
        }
    }

    auto env            = Env{};
    env.strings_table   = std::move(strings);
    env.functions_table = std::move(fn_table);
    env.ip_base         = &text[0];

    auto stack = Stack{env};
    stack.push(256, (text.data() + entry_addr), nullptr);

    auto const ip_begin = &text[0];
    auto const ip_end   = (ip_begin + text.size());

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
        run(stack,
            stack.back().entry_address,
            {(executable_path + "[.text]"), ip_begin, ip_end});
    } else {
        try {
            run(stack,
                stack.back().entry_address,
                {(executable_path + "[.text]"), ip_begin, ip_end});
        } catch (viua::vm::abort_execution const& e) {
            std::cerr << "Aborted execution: " << e.what() << "\n";
            return 1;
        }
    }

    return 0;
}
