/*
 *  Copyright (C) 2015, 2016 Marek Marecki
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

#include <memory>
#include <functional>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/boolean.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/assert.h>
using namespace std;


byte* viua::process::Process::opizero(byte* addr) {
    unsigned target = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    place(target, unique_ptr<viua::types::Type>{new viua::types::Integer(0)});
    return addr;
}

byte* viua::process::Process::opistore(byte* addr) {
    unsigned target = 0;
    int integer = 0;

    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, integer) = viua::bytecode::decoder::operands::fetch_primitive_uint(addr, this);

    place(target, unique_ptr<viua::types::Type>{new viua::types::Integer(integer)});

    return addr;
}

template<class Operator, class ResultType> byte* perform(byte* addr, viua::process::Process* t) {
    /** Heavily abstracted binary opcode implementation for Integer-related instructions.
     *
     *  First parameter - byte* addr - is the instruction pointer from which operand extraction should begin.
     *
     *  Second parameter - viua::process::Process* t - is a pointer to current VM process (passed as `this`).
     */
    unsigned target_register_index = 0;
    tie(addr, target_register_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, t);

    viua::types::Type *first = nullptr, *second = nullptr;
    tie(addr, first) = viua::bytecode::decoder::operands::fetch_object(addr, t);
    tie(addr, second) = viua::bytecode::decoder::operands::fetch_object(addr, t);

    using viua::types::numeric::Number;

    viua::assertions::expect_types<Number>("Number", first, second);

    // FIXME use 64 bit integers by default
    t->put(target_register_index, unique_ptr<viua::types::Type>{new ResultType(Operator()(dynamic_cast<Number*>(first)->as_int32(), dynamic_cast<Number*>(second)->as_int32()))});

    return addr;
}

byte* viua::process::Process::opiadd(byte* addr) {
    return perform<std::plus<int>, viua::types::Integer>(addr, this);
}

byte* viua::process::Process::opisub(byte* addr) {
    return perform<std::minus<int>, viua::types::Integer>(addr, this);
}

byte* viua::process::Process::opimul(byte* addr) {
    return perform<std::multiplies<int>, viua::types::Integer>(addr, this);
}

byte* viua::process::Process::opidiv(byte* addr) {
    return perform<std::divides<int>, viua::types::Integer>(addr, this);
}

byte* viua::process::Process::opilt(byte* addr) {
    return perform<std::less<int>, viua::types::Boolean>(addr, this);
}

byte* viua::process::Process::opilte(byte* addr) {
    return perform<std::less_equal<int>, viua::types::Boolean>(addr, this);
}

byte* viua::process::Process::opigt(byte* addr) {
    return perform<std::greater<int>, viua::types::Boolean>(addr, this);
}

byte* viua::process::Process::opigte(byte* addr) {
    return perform<std::greater_equal<int>, viua::types::Boolean>(addr, this);
}

byte* viua::process::Process::opieq(byte* addr) {
    return perform<std::equal_to<int>, viua::types::Boolean>(addr, this);
}

byte* viua::process::Process::opiinc(byte* addr) {
    viua::types::Type* target { nullptr };
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    viua::assertions::expect_type<viua::types::Integer>("Integer", target)->increment();

    return addr;
}

byte* viua::process::Process::opidec(byte* addr) {
    viua::types::Type* target { nullptr };
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    viua::assertions::expect_type<viua::types::Integer>("Integer", target)->decrement();

    return addr;
}
