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

#ifndef VIUA_VM_INS_H
#define VIUA_VM_INS_H

#include <vector>

#include <viua/arch/ins.h>
#include <viua/vm/core.h>


namespace viua::vm::ins {
#define Base_instruction(it)                \
    auto execute(viua::arch::ins::it const, \
                 Stack&,                    \
                 viua::arch::instruction_type const* const)

#define Work_instruction(it) Base_instruction(it)->void
#define Flow_instruction(it) \
    Base_instruction(it)->viua::arch::instruction_type const*


Work_instruction(ADD);
Work_instruction(SUB);
Work_instruction(MUL);
Work_instruction(DIV);
Work_instruction(MOD);

Work_instruction(BITSHL);
Work_instruction(BITSHR);
Work_instruction(BITASHR);
Work_instruction(BITROL);
Work_instruction(BITROR);
Work_instruction(BITAND);
Work_instruction(BITOR);
Work_instruction(BITXOR);
Work_instruction(BITNOT);

Work_instruction(EQ);
Work_instruction(LT);
Work_instruction(GT);
Work_instruction(CMP);
Work_instruction(AND);
Work_instruction(OR);
Work_instruction(NOT);

Work_instruction(COPY);
Work_instruction(MOVE);
Work_instruction(SWAP);

Work_instruction(ATOM);
Work_instruction(STRING);
Work_instruction(STRUCT);
Work_instruction(BUFFER);

Work_instruction(FRAME);
Flow_instruction(CALL);
Flow_instruction(RETURN);

Work_instruction(LUI);
Work_instruction(LUIU);

Work_instruction(FLOAT);
Work_instruction(DOUBLE);

Work_instruction(ADDI);
Work_instruction(ADDIU);
Work_instruction(SUBI);
Work_instruction(SUBIU);
Work_instruction(MULI);
Work_instruction(MULIU);
Work_instruction(DIVI);
Work_instruction(DIVIU);

Work_instruction(BUFFER_PUSH);
Work_instruction(BUFFER_SIZE);
Work_instruction(BUFFER_AT);
Work_instruction(BUFFER_POP);

Work_instruction(STRUCT_AT);
Work_instruction(STRUCT_INSERT);
Work_instruction(STRUCT_REMOVE);

Work_instruction(REF);

Flow_instruction(IF);

Work_instruction(EBREAK);
Work_instruction(ECALL);

Work_instruction(IO_SUBMIT);
Work_instruction(IO_WAIT);
Work_instruction(IO_SHUTDOWN);
Work_instruction(IO_CTL);
Work_instruction(IO_PEEK);

Work_instruction(ACTOR);
Work_instruction(SELF);

constexpr auto VIUA_TRACE_CYCLES = true;
auto execute(viua::vm::Stack&, viua::arch::instruction_type const* const)
    -> viua::arch::instruction_type const*;

/*
 * Utility functions. Used in implementation of EBREAK, but also accessed by the
 * repl-debugger combo to dump backtraces and register dumps. Reuse makes
 * keeping the format consistent easier.
 */
auto print_backtrace(viua::vm::Stack const&,
                     std::optional<size_t> const = std::nullopt) -> void;
auto dump_registers(std::vector<Value> const&, std::string_view const) -> void;
}  // namespace viua::vm::ins

#endif
