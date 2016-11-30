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

#include <functional>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/types/type.h>
#include <viua/types/boolean.h>
#include <viua/types/integer.h>
#include <viua/types/float.h>
#include <viua/kernel/kernel.h>
#include <viua/assert.h>
using namespace std;


byte* viua::process::Process::opfstore(byte* addr) {
    /*  Run fstore instruction.
     */
    unsigned target = 0;
    float value = 0.0;

    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, value) = viua::bytecode::decoder::operands::fetch_raw_float(addr, this);

    place(target, unique_ptr<viua::types::Type>{new viua::types::Float(value)});

    return addr;
}

template<class Operator, class ResultType> byte* perform(byte* addr, viua::process::Process* t) {
    /** Heavily abstracted binary opcode implementation for Float-related instructions.
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

    t->put(target_register_index, unique_ptr<viua::types::Type>{new ResultType(Operator()(dynamic_cast<Number*>(first)->as_float32(), dynamic_cast<Number*>(second)->as_float32()))});

    return addr;
}

byte* viua::process::Process::opfadd(byte* addr) {
    return perform<std::plus<float>, viua::types::Float>(addr, this);
}

byte* viua::process::Process::opfsub(byte* addr) {
    return perform<std::minus<float>, viua::types::Float>(addr, this);
}

byte* viua::process::Process::opfmul(byte* addr) {
    return perform<std::multiplies<float>, viua::types::Float>(addr, this);
}

byte* viua::process::Process::opfdiv(byte* addr) {
    return perform<std::divides<float>, viua::types::Float>(addr, this);
}

byte* viua::process::Process::opflt(byte* addr) {
    return perform<std::less<float>, viua::types::Boolean>(addr, this);
}

byte* viua::process::Process::opflte(byte* addr) {
    return perform<std::less_equal<float>, viua::types::Boolean>(addr, this);
}

byte* viua::process::Process::opfgt(byte* addr) {
    return perform<std::greater<float>, viua::types::Boolean>(addr, this);
}

byte* viua::process::Process::opfgte(byte* addr) {
    return perform<std::greater_equal<float>, viua::types::Boolean>(addr, this);
}

byte* viua::process::Process::opfeq(byte* addr) {
    return perform<std::equal_to<float>, viua::types::Boolean>(addr, this);
}
