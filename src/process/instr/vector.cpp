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

#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/assert.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/vector.h>
#include <viua/kernel/opex.h>
#include <viua/kernel/registerset.h>
#include <viua/operand.h>
#include <viua/kernel/kernel.h>
using namespace std;


byte* Process::opvec(byte* addr) {
    unsigned register_index = 0, pack_start_index = 0, pack_length = 0;
    tie(addr, register_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, pack_start_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, pack_length) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    if ((register_index > pack_start_index) and (register_index < (pack_start_index+pack_length))) {
        // FIXME vector is inserted into a register after packing, so this exception is not entirely well thought-out
        // allow packing target register
        throw new Exception("vec would pack itself");
    }
    if ((pack_start_index+pack_length) >= uregset->size()) {
        throw new Exception("vec: packing outside of register set range");
    }
    for (decltype(pack_length) i = 0; i < pack_length; ++i) {
        if (uregset->at(pack_start_index+i) == nullptr) {
            throw new Exception("vec: cannot pack null register");
        }
    }

    unique_ptr<Vector> v(new Vector());
    for (decltype(pack_length) i = 0; i < pack_length; ++i) {
        v->push(pop(pack_start_index+i));
    }

    place(register_index, v.release());

    return addr;
}

byte* Process::opvinsert(byte* addr) {
    /*  Run vinsert instruction.
     */
    Type* vector_operand = viua::operand::extract(addr)->resolve(this);
    unsigned object_operand_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    unsigned position_operand_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    viua::assertions::assert_implements<Vector>(vector_operand, "Vector");

    static_cast<Vector*>(vector_operand)->insert(position_operand_index, pop(object_operand_index));

    return addr;
}

byte* Process::opvpush(byte* addr) {
    /*  Run vpush instruction.
     *
     *  Vector always pushes a copy of the object in a register.
     *  FIXME: make it possible to push references.
     */
    Type* target = viua::operand::extract(addr)->resolve(this);
    unsigned source = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    viua::assertions::assert_implements<Vector>(target, "Vector");
    static_cast<Vector*>(target)->push(pop(source));

    return addr;
}

byte* Process::opvpop(byte* addr) {
    /*  Run vpop instruction.
     */
    unsigned destination_register_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    Type* vector_operand = viua::operand::extract(addr)->resolve(this);

    int position_operand_index = 0;
    bool reg_ref = false;

    viua::kernel::util::extractIntegerOperand(addr, reg_ref, position_operand_index);

    if (reg_ref) {
        // register index references cannot be negative so it's safe to cast to unsigned
        position_operand_index = static_cast<Integer*>(fetch(static_cast<unsigned>(position_operand_index)))->value();
    }

    viua::assertions::assert_implements<Vector>(vector_operand, "Vector");
    /*  1) fetch vector,
     *  2) pop value at given index,
     *  3) put it in a register,
     */
    Type* ptr = static_cast<Vector*>(vector_operand)->pop(position_operand_index);
    place(destination_register_index, ptr);

    return addr;
}

byte* Process::opvat(byte* addr) {
    /*  Run vat instruction.
     *
     *  Vector always returns a copy of the object in a register.
     */
    unsigned destination_register_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    Type* vector_operand = viua::operand::extract(addr)->resolve(this);

    int position_operand_index = 0;
    bool reg_ref = false;

    viua::kernel::util::extractIntegerOperand(addr, reg_ref, position_operand_index);

    if (reg_ref) {
        // register index references cannot be negative so it's safe to cast to unsigned
        position_operand_index = static_cast<Integer*>(fetch(static_cast<unsigned>(position_operand_index)))->value();
    }

    viua::assertions::assert_implements<Vector>(vector_operand, "Vector");
    Type* ptr = static_cast<Vector*>(vector_operand)->at(position_operand_index);
    place(destination_register_index, ptr->copy());

    return addr;
}

byte* Process::opvlen(byte* addr) {
    /*  Run vlen instruction.
     */
    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    Type* source = viua::operand::extract(addr)->resolve(this);

    viua::assertions::assert_implements<Vector>(source, "Vector");
    place(target, new Integer(static_cast<Vector*>(source)->len()));

    return addr;
}
