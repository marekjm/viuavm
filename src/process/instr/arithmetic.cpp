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

#include <iostream>
#include <memory>
#include <functional>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/types/type.h>
#include <viua/types/number.h>
#include <viua/types/integer.h>
#include <viua/types/float.h>
#include <viua/types/boolean.h>
#include <viua/types/exception.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/assert.h>
using namespace std;

using ArithmeticOp = unique_ptr<viua::types::numeric::Number>(viua::types::numeric::Number::*)(const viua::types::numeric::Number&) const;
using LogicOp = unique_ptr<viua::types::Boolean>(viua::types::numeric::Number::*)(const viua::types::numeric::Number&) const;

template < typename OpType, OpType action > static auto alu_impl(viua::internals::types::byte* addr, viua::process::Process *process) -> viua::internals::types::byte* {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, process);

    viua::types::Type* lhs_raw = nullptr;
    tie(addr, lhs_raw) = viua::bytecode::decoder::operands::fetch_object2(addr, process);

    viua::types::Type* rhs_raw = nullptr;
    tie(addr, rhs_raw) = viua::bytecode::decoder::operands::fetch_object2(addr, process);

    using viua::types::numeric::Number;

    viua::assertions::expect_types<Number>("Number", lhs_raw, rhs_raw);

    auto lhs = static_cast<viua::types::numeric::Number*>(lhs_raw);
    auto rhs = static_cast<viua::types::numeric::Number*>(rhs_raw);

    *target = (lhs->*action)(*rhs);

    return addr;
}

viua::internals::types::byte* viua::process::Process::opadd(viua::internals::types::byte* addr) {
    return alu_impl<ArithmeticOp, &viua::types::numeric::Number::operator+>(addr, this);
}

viua::internals::types::byte* viua::process::Process::opsub(viua::internals::types::byte* addr) {
    return alu_impl<ArithmeticOp, &viua::types::numeric::Number::operator- >(addr, this);
}

viua::internals::types::byte* viua::process::Process::opmul(viua::internals::types::byte* addr) {
    return alu_impl<ArithmeticOp, &viua::types::numeric::Number::operator*>(addr, this);
}

viua::internals::types::byte* viua::process::Process::opdiv(viua::internals::types::byte* addr) {
    return alu_impl<ArithmeticOp, &viua::types::numeric::Number::operator/>(addr, this);
}

viua::internals::types::byte* viua::process::Process::oplt(viua::internals::types::byte* addr) {
    return alu_impl<LogicOp, &viua::types::numeric::Number::operator< >(addr, this);
}

viua::internals::types::byte* viua::process::Process::oplte(viua::internals::types::byte* addr) {
    return alu_impl<LogicOp, &viua::types::numeric::Number::operator<= >(addr, this);
}

viua::internals::types::byte* viua::process::Process::opgt(viua::internals::types::byte* addr) {
    return alu_impl<LogicOp, &viua::types::numeric::Number::operator> >(addr, this);
}

viua::internals::types::byte* viua::process::Process::opgte(viua::internals::types::byte* addr) {
    return alu_impl<LogicOp, &viua::types::numeric::Number::operator>= >(addr, this);
}

viua::internals::types::byte* viua::process::Process::opeq(viua::internals::types::byte* addr) {
    return alu_impl<LogicOp, &viua::types::numeric::Number::operator== >(addr, this);
}
