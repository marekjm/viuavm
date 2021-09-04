/*
 *  Copyright (C) 2021 Marek Marecki
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
#define Base_instruction(it) \
    auto execute(viua::arch::ins::it const, Stack&, viua::arch::instruction_type const* const)

#define Work_instruction(it) \
    Base_instruction(it) -> void
#define Flow_instruction(it) \
    Base_instruction(it) -> viua::arch::instruction_type const*


Work_instruction(ADD);
Work_instruction(SUB);
Work_instruction(MUL);
Work_instruction(DIV);
Work_instruction(MOD);

auto execute(std::vector<Value>& registers, viua::arch::ins::BITSHL const op) -> void;
auto execute(std::vector<Value>& registers, viua::arch::ins::BITSHR const op) -> void;
auto execute(std::vector<Value>& registers, viua::arch::ins::BITASHR const op) -> void;
auto execute(std::vector<Value>& registers, viua::arch::ins::BITROL const op) -> void;
auto execute(std::vector<Value>& registers, viua::arch::ins::BITROR const op) -> void;
auto execute(std::vector<Value>& registers, viua::arch::ins::BITAND const op) -> void;
auto execute(std::vector<Value>& registers, viua::arch::ins::BITOR const op) -> void;
auto execute(std::vector<Value>& registers, viua::arch::ins::BITXOR const op) -> void;
auto execute(std::vector<Value>& registers, viua::arch::ins::BITNOT const op) -> void;

auto execute(std::vector<Value>& registers, viua::arch::ins::EQ const op) -> void;
auto execute(std::vector<Value>& registers, viua::arch::ins::LT const op) -> void;
auto execute(std::vector<Value>& registers, viua::arch::ins::GT const op) -> void;
auto execute(std::vector<Value>& registers, viua::arch::ins::CMP const op) -> void;
auto execute(std::vector<Value>& registers, viua::arch::ins::AND const op) -> void;
auto execute(std::vector<Value>& registers, viua::arch::ins::OR const op) -> void;
auto execute(std::vector<Value>& registers, viua::arch::ins::NOT const op) -> void;

auto execute(std::vector<Value>& registers,
             viua::arch::ins::DELETE const op) -> void;

auto execute(std::vector<Value>& registers,
             Env const& env,
             viua::arch::ins::STRING const op) -> void;

auto execute(Stack& stack,
             viua::arch::instruction_type const* const ip,
             viua::arch::ins::FRAME const op) -> void;

auto execute(Stack& stack,
             Env const& env,
             viua::arch::instruction_type const* const ip,
             viua::arch::ins::CALL const op) -> viua::arch::instruction_type const*;

auto execute(std::vector<Value>& registers,
             viua::arch::ins::LUI const op) -> void;
auto execute(std::vector<Value>& registers,
             viua::arch::ins::LUIU const op) -> void;

auto execute(Stack& stack,
             viua::arch::instruction_type const* const,
             Env const& env,
             viua::arch::ins::FLOAT const op) -> void;
auto execute(Stack& stack,
             viua::arch::instruction_type const* const,
             Env const& env,
             viua::arch::ins::DOUBLE const op) -> void;

auto execute(std::vector<Value>& registers,
             viua::arch::ins::ADDI const op) -> void;
auto execute(std::vector<Value>& registers,
             viua::arch::ins::ADDIU const op) -> void;
auto execute(std::vector<Value>& registers,
             viua::arch::ins::SUBI const op) -> void;
auto execute(std::vector<Value>& registers,
             viua::arch::ins::SUBIU const op) -> void;
auto execute(std::vector<Value>& registers,
             viua::arch::ins::MULI const op) -> void;
auto execute(std::vector<Value>& registers,
             viua::arch::ins::MULIU const op) -> void;
auto execute(std::vector<Value>& registers,
             viua::arch::ins::DIVI const op) -> void;
auto execute(std::vector<Value>& registers,
             viua::arch::ins::DIVIU const op) -> void;

auto execute(Stack const& stack,
             Env const&,
             viua::arch::ins::EBREAK const) -> void;

auto execute(Stack& stack,
             viua::arch::instruction_type const* const ip,
             Env const&,
             viua::arch::ins::RETURN const ins) -> viua::arch::instruction_type const*;
}

#endif
