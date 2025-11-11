/*
 *  Copyright (C) 2021-2025 Marek Marecki
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
#include <string.h>

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <print>
#include <utility>
#include <vector>

#include <viua/arch/arch.h>
#include <viua/support/fdstream.h>
#include <viua/vm/backtrace.h>
#include <viua/vm/ins.h>


namespace viua {
extern viua::support::fdstream TRACE_STREAM;
}

namespace viua::vm::ins {
using namespace viua::arch::ins;
using viua::vm::Stack;

#define Work(FORMAT, OP)                       \
    case OPCODE_##FORMAT::OP:                  \
        execute(OP{ instruction }, stack, ip); \
        break

#define Flow(FORMAT, OP)      \
    case OPCODE_##FORMAT::OP: \
        return execute(OP{ instruction }, stack, ip)

#define Intro(FORMAT)                                           \
    auto instruction = viua::arch::ops::FORMAT::decode(raw);    \
    if constexpr (VIUA_TRACE_CYCLES) {                          \
        viua::TRACE_STREAM << "    " << instruction.to_string() \
                           << viua::TRACE_STREAM.endl;          \
    }                                                           \
    using viua::arch::ops::OPCODE_##FORMAT;                     \
    switch (static_cast<OPCODE_##FORMAT>(opcode))

auto execute(
    viua::vm::Stack& stack,
    viua::arch::instruction_type const* const ip)
    -> viua::arch::instruction_type const*
{
    auto const raw = *ip;

    auto const opcode = carve_just_opcode_out(raw);
    auto const format = carve_format_out(opcode);

    switch (format) {
        using viua::vm::ins::execute;
        using namespace viua::arch::ins;
        using enum viua::arch::ops::FORMAT;
        case T:
            {
                Intro(T)
                {
                    Work(T, ADD);
                    Work(T, SUB);
                    Work(T, MUL);
                    Work(T, DIV);
                    Work(T, MOD);
                    Work(T, BITSHL);
                    Work(T, BITSHR);
                    Work(T, BITASHR);
                    Work(T, BITROL);
                    Work(T, BITROR);
                    Work(T, BITAND);
                    Work(T, BITOR);
                    Work(T, BITXOR);
                    Work(T, EQ);
                    Work(T, GT);
                    Work(T, LT);
                    Work(T, CMP);
                    Work(T, AND);
                    Work(T, OR);
                    Work(T, IO_SUBMIT);
                    Work(T, IO_WAIT);
                    Work(T, IO_SHUTDOWN);
                    Work(T, IO_CTL);
                    Work(T, MOVEIF);
                }
                break;
            }
        case S:
            {
                Intro(S)
                {
                    Work(S, FRAME);
                    Flow(S, RETURN);
                    Work(S, ATOM);
                    Work(S, DOUBLE);
                    Work(S, SELF);
                }
                break;
            }
        case I:
            {
                Intro(I)
                {
                    Work(I, LUI);
                    Work(I, LUIU);
                    Work(I, LLI);
                    Work(I, FLOAT);
                    Work(I, CAST);
                    Work(I, ARODP);
                    Work(I, ATXTP);
                }
                break;
            }
        case U:
            {
                Intro(U)
                {
                    Work(U, ADDI);
                    Work(U, ADDIU);
                    Work(U, SUBI);
                    Work(U, SUBIU);
                    Work(U, MULI);
                    Work(U, MULIU);
                    Work(U, DIVI);
                    Work(U, DIVIU);
                }
                break;
            }
        case N:
            {
                if constexpr (VIUA_TRACE_CYCLES) {
                    viua::TRACE_STREAM
                        << "    "
                        << viua::arch::ops::to_string(
                               static_cast<viua::arch::opcode_type>(opcode))
                        << viua::TRACE_STREAM.endl;
                }

                using viua::arch::ops::OPCODE_N;
                switch (static_cast<OPCODE_N>(opcode)) {
                    case OPCODE_N::NOOP:
                        break;
                    case OPCODE_N::HALT:
                        return nullptr;
                    case OPCODE_N::EBREAK:
                        execute(EBREAK{ viua::arch::ops::N::decode(raw) },
                                stack,
                                ip);
                        break;
                    case OPCODE_N::ECALL:
                        execute(ECALL{ viua::arch::ops::N::decode(raw) },
                                stack,
                                ip);
                        break;
                }
                break;
            }
        case M:
            {
                Intro(M)
                {
                    Work(M, SM);
                    Work(M, LM);
                    Work(M, AA);
                    Work(M, AD);
                    Work(M, PTR);
                }
                break;
            }
        case D:
            {
                Intro(D)
                {
                    /*
                     * Call is a special instruction. It transfers the IP to a
                     * semi-random location, instead of just increasing it to
                     * the next unit.
                     *
                     * This is why we return here, and not use the default
                     * behaviour for most of the other instructions.
                     */
                    Flow(D, CALL);
                    Work(D, BITNOT);
                    Work(D, NOT);
                    Work(D, COPY);
                    Work(D, MOVE);
                    Work(D, SWAP);
                    /*
                     * If is a special instruction. It transfers IP to a
                     * semi-random location instead of just increasing it to the
                     * next unit. This is why the return is used instead of
                     * break.
                     */
                    Flow(D, IF);
                    Work(D, IO_PEEK);
                    Work(D, ACTOR);
                    Work(D, GTS);
                    Work(D, GTL);
                    Work(D, EARITHMETICWIDTH);

                    Work(D, BITREV);
                    Work(D, BITAREV);
                }
                break;
            }
        default:
            std::println(stderr,
                         "unimplemented instruction: {}",
                         viua::arch::ops::to_string(
                             static_cast<viua::arch::opcode_type>(opcode)));
            return nullptr;
    }

    return (ip + 1);
}
}  // namespace viua::vm::ins
