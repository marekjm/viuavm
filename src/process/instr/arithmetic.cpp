/*
 *  Copyright (C) 2016, 2017 Marek Marecki
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

#include <functional>
#include <iostream>
#include <memory>
#include <viua/assert.h>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/types/boolean.h>
#include <viua/types/exception.h>
#include <viua/types/float.h>
#include <viua/types/integer.h>
#include <viua/types/number.h>
#include <viua/types/value.h>
using namespace std;

using viua::types::numeric::Number;
using ArithmeticOp = unique_ptr<Number> (Number::*)(const Number&) const;
using LogicOp = unique_ptr<viua::types::Boolean> (Number::*)(const Number&) const;

template<typename OpType, OpType action>
static auto alu_impl(viua::internals::types::byte* addr, viua::process::Process* process)
    -> viua::internals::types::byte* {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, process);

    Number* lhs = nullptr;
    tie(addr, lhs) = viua::bytecode::decoder::operands::fetch_object_of<Number>(addr, process);

    Number* rhs = nullptr;
    tie(addr, rhs) = viua::bytecode::decoder::operands::fetch_object_of<Number>(addr, process);

    *target = (lhs->*action)(*rhs);

    return addr;
}

viua::internals::types::byte* viua::process::Process::opadd(viua::internals::types::byte* addr) {
    return alu_impl<ArithmeticOp, (&Number::operator+)>(addr, this);
}

viua::internals::types::byte* viua::process::Process::opsub(viua::internals::types::byte* addr) {
    return alu_impl<ArithmeticOp, (&Number::operator-)>(addr, this);
}

viua::internals::types::byte* viua::process::Process::opmul(viua::internals::types::byte* addr) {
    return alu_impl<ArithmeticOp, (&Number::operator*)>(addr, this);
}

viua::internals::types::byte* viua::process::Process::opdiv(viua::internals::types::byte* addr) {
    return alu_impl<ArithmeticOp, (&Number::operator/)>(addr, this);
}

viua::internals::types::byte* viua::process::Process::oplt(viua::internals::types::byte* addr) {
    return alu_impl<LogicOp, (&Number::operator<)>(addr, this);
}

viua::internals::types::byte* viua::process::Process::oplte(viua::internals::types::byte* addr) {
    return alu_impl<LogicOp, (&Number::operator<=)>(addr, this);
}

viua::internals::types::byte* viua::process::Process::opgt(viua::internals::types::byte* addr) {
    return alu_impl<LogicOp, ((&Number::operator>))>(addr, this);
}

viua::internals::types::byte* viua::process::Process::opgte(viua::internals::types::byte* addr) {
    return alu_impl<LogicOp, (&Number::operator>=)>(addr, this);
}

viua::internals::types::byte* viua::process::Process::opeq(viua::internals::types::byte* addr) {
    return alu_impl<LogicOp, (&Number::operator==)>(addr, this);
}
