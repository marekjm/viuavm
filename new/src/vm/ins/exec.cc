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

auto execute(viua::vm::Stack& stack,
             viua::arch::instruction_type const* const ip)
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
#define Work(OP)                             \
    case OPCODE_T::OP:                       \
        execute(OP{instruction}, stack, ip); \
        break
            Work(ADD);
            Work(SUB);
            Work(MUL);
            Work(DIV);
            Work(MOD);
            Work(BITSHL);
            Work(BITSHR);
            Work(BITASHR);
            Work(BITROL);
            Work(BITROR);
            Work(BITAND);
            Work(BITOR);
            Work(BITXOR);
            Work(EQ);
            Work(GT);
            Work(LT);
            Work(CMP);
            Work(AND);
            Work(OR);
            Work(IO_SUBMIT);
            Work(IO_WAIT);
            Work(IO_SHUTDOWN);
            Work(IO_CTL);
#undef Work
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
#define Work(OP)                             \
    case OPCODE_S::OP:                       \
        execute(OP{instruction}, stack, ip); \
        break
#define Flow(OP)       \
    case OPCODE_S::OP: \
        return execute(OP{instruction}, stack, ip)
            Work(FRAME);
            Flow(RETURN);
            Work(ATOM);
            Work(DOUBLE);
            Work(SELF);
#undef Work
#undef Flow
        }
        break;
    }
    case F:
    {
        auto instruction = viua::arch::ops::F::decode(raw);
        if constexpr (VIUA_TRACE_CYCLES) {
            viua::TRACE_STREAM << "    " << instruction.to_string()
                               << viua::TRACE_STREAM.endl;
        }

        using viua::arch::ops::OPCODE_F;
        switch (static_cast<OPCODE_F>(opcode)) {
#define Work(OP)                             \
    case OPCODE_F::OP:                       \
        execute(OP{instruction}, stack, ip); \
        break
            Work(LUI);
            Work(LUIU);
            Work(LLI);
            Work(FLOAT);
#undef Work
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
#define Work(OP)                             \
    case OPCODE_E::OP:                       \
        execute(OP{instruction}, stack, ip); \
        break
            Work(CAST);
            Work(ARODP);
            Work(ATXTP);
#undef Work
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
#define Work(OP)                             \
    case OPCODE_R::OP:                       \
        execute(OP{instruction}, stack, ip); \
        break
            Work(ADDI);
            Work(ADDIU);
            Work(SUBI);
            Work(SUBIU);
            Work(MULI);
            Work(MULIU);
            Work(DIVI);
            Work(DIVIU);
#undef Work
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
            break;
        case OPCODE_N::ECALL:
            execute(ECALL{viua::arch::ops::N::decode(raw)}, stack, ip);
            break;
        }
        break;
    }
    case M:
    {
        auto instruction = viua::arch::ops::M::decode(raw);
        if constexpr (VIUA_TRACE_CYCLES) {
            viua::TRACE_STREAM << "    " << instruction.to_string()
                               << viua::TRACE_STREAM.endl;
        }

        using viua::arch::ops::OPCODE_M;
        switch (static_cast<OPCODE_M>(opcode)) {
#define Work(OP)                             \
    case OPCODE_M::OP:                       \
        execute(OP{instruction}, stack, ip); \
        break
            Work(SM);
            Work(LM);
            Work(AA);
            Work(AD);
            Work(PTR);
#undef Work
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
#define Work(OP)                             \
    case OPCODE_D::OP:                       \
        execute(OP{instruction}, stack, ip); \
        break
#define Flow(OP)       \
    case OPCODE_D::OP: \
        return execute(OP{instruction}, stack, ip)
            /*
             * Call is a special instruction. It transfers the IP to a
             * semi-random location, instead of just increasing it to the next
             * unit.
             *
             * This is why we return here, and not use the default behaviour for
             * most of the other instructions.
             */
            Flow(CALL);
            Work(BITNOT);
            Work(NOT);
            Work(COPY);
            Work(MOVE);
            Work(SWAP);
            /*
             * If is a special instruction. It transfers IP to a semi-random
             * location instead of just increasing it to the next unit. This is
             * why the return is used instead of break.
             */
            Flow(IF);
            Work(IO_PEEK);
            Work(ACTOR);
            Work(GTS);
            Work(GTL);
#undef Work
#undef Flow
        }
        break;
    }
    default:
        std::cerr << "unimplemented instruction: "
                  << viua::arch::ops::to_string(opcode) << "\n";
        return nullptr;
    }

    return (ip + 1);
}
}
