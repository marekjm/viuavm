/*
 *  Copyright (C) 2016, 2017, 2018 Marek Marecki
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
using ArithmeticOp = std::unique_ptr<Number> (Number::*)(const Number&) const;
using LogicOp =
    std::unique_ptr<viua::types::Boolean> (Number::*)(const Number&) const;
using viua::internals::types::Op_address_type;

template<typename T>
using dumb_ptr = T*;  // FIXME; use std::experimental::observer_ptr

template<typename OpType, OpType action>
static auto alu_impl(Op_address_type addr, viua::process::Process* process)
    -> Op_address_type {
    auto target = dumb_ptr<viua::kernel::Register>{nullptr};
    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, process);

    auto lhs       = dumb_ptr<Number>{nullptr};
    tie(addr, lhs) = viua::bytecode::decoder::operands::fetch_object_of<Number>(
        addr, process);

    auto rhs       = dumb_ptr<Number>{nullptr};
    tie(addr, rhs) = viua::bytecode::decoder::operands::fetch_object_of<Number>(
        addr, process);

    *target = (lhs->*action)(*rhs);

    return addr;
}

auto viua::process::Process::opadd(Op_address_type addr) -> Op_address_type {
    return alu_impl<ArithmeticOp, (&Number::operator+)>(addr, this);
}

auto viua::process::Process::opsub(Op_address_type addr) -> Op_address_type {
    return alu_impl<ArithmeticOp, (&Number::operator-)>(addr, this);
}

auto viua::process::Process::opmul(Op_address_type addr) -> Op_address_type {
    return alu_impl<ArithmeticOp, (&Number::operator*)>(addr, this);
}

auto viua::process::Process::opdiv(Op_address_type addr) -> Op_address_type {
    return alu_impl<ArithmeticOp, (&Number::operator/)>(addr, this);
}

auto viua::process::Process::oplt(Op_address_type addr) -> Op_address_type {
    return alu_impl<LogicOp, (&Number::operator<)>(addr, this);
}

auto viua::process::Process::oplte(Op_address_type addr) -> Op_address_type {
    return alu_impl<LogicOp, (&Number::operator<=)>(addr, this);
}

auto viua::process::Process::opgt(Op_address_type addr) -> Op_address_type {
    return alu_impl<LogicOp, ((&Number::operator>))>(addr, this);
}

auto viua::process::Process::opgte(Op_address_type addr) -> Op_address_type {
    return alu_impl<LogicOp, (&Number::operator>=)>(addr, this);
}

auto viua::process::Process::opeq(Op_address_type addr) -> Op_address_type {
    return alu_impl<LogicOp, (&Number::operator==)>(addr, this);
}
