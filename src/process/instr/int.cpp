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


byte* Process::opizero(byte* addr) {
    unsigned target = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    place(target, new Integer(0));
    return addr;
}

byte* Process::opistore(byte* addr) {
    unsigned target = 0;
    int integer = 0;

    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, integer) = viua::bytecode::decoder::operands::fetch_primitive_uint(addr, this);

    place(target, new Integer(integer));

    return addr;
}

template<class Operator, class ResultType> byte* perform(byte* addr, Process* t) {
    /** Heavily abstracted binary opcode implementation for Integer-related instructions.
     *
     *  First parameter - byte* addr - is the instruction pointer from which operand extraction should begin.
     *
     *  Second parameter - Process* t - is a pointer to current VM process (passed as `this`).
     *
     *  Third parameter - ObjectPlacer - is a member-function pointer to Process::place.
     *  Since it is private, we have to cheat the compiler by extracting its pointer while in
     *  Process class's scope and passing it here.
     *  Voila - we can place objects in process's current register set.
     */
    unsigned target_register_index = 0, first_operand_index = 0, second_operand_index = 0;
    tie(addr, target_register_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, t);
    tie(addr, first_operand_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, t);
    tie(addr, second_operand_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, t);

    auto first = t->obtain(first_operand_index);
    auto second = t->obtain(second_operand_index);

    using viua::types::numeric::Number;

    viua::assertions::expect_types<Number>("Number", first, second);

    // FIXME use 64 bit integers by default
    t->put(target_register_index, new ResultType(Operator()(dynamic_cast<Number*>(first)->as_int32(), dynamic_cast<Number*>(second)->as_int32())));

    return addr;
}

byte* Process::opiadd(byte* addr) {
    return perform<std::plus<int>, Integer>(addr, this);
}

byte* Process::opisub(byte* addr) {
    return perform<std::minus<int>, Integer>(addr, this);
}

byte* Process::opimul(byte* addr) {
    return perform<std::multiplies<int>, Integer>(addr, this);
}

byte* Process::opidiv(byte* addr) {
    return perform<std::divides<int>, Integer>(addr, this);
}

byte* Process::opilt(byte* addr) {
    return perform<std::less<int>, Boolean>(addr, this);
}

byte* Process::opilte(byte* addr) {
    return perform<std::less_equal<int>, Boolean>(addr, this);
}

byte* Process::opigt(byte* addr) {
    return perform<std::greater<int>, Boolean>(addr, this);
}

byte* Process::opigte(byte* addr) {
    return perform<std::greater_equal<int>, Boolean>(addr, this);
}

byte* Process::opieq(byte* addr) {
    return perform<std::equal_to<int>, Boolean>(addr, this);
}

byte* Process::opiinc(byte* addr) {
    unsigned target = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    viua::assertions::expect_type<Integer>("Integer", fetch(target))->increment();

    return addr;
}

byte* Process::opidec(byte* addr) {
    unsigned target = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    viua::assertions::expect_type<Integer>("Integer", fetch(target))->decrement();

    return addr;
}
