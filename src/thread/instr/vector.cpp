#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/vector.h>
#include <viua/cpu/opex.h>
#include <viua/cpu/registerset.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::vec(byte* addr) {
    place(viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this), new Vector());

    return addr;
}

byte* Thread::vinsert(byte* addr) {
    /*  Run vinsert instruction.
     *
     *  Vector always inserts a copy of the object in a register.
     *  FIXME: make it possible to insert references.
     */
    int vector_operand_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    int object_operand_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    int position_operand_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    static_cast<Vector*>(fetch(vector_operand_index))->insert(position_operand_index, fetch(object_operand_index)->copy());

    return addr;
}

byte* Thread::vpush(byte* addr) {
    /*  Run vpush instruction.
     *
     *  Vector always pushes a copy of the object in a register.
     *  FIXME: make it possible to push references.
     */
    Type* target = viua::operand::extract(addr)->resolve(this);
    Type* source = viua::operand::extract(addr)->resolve(this);

    static_cast<Vector*>(target)->push(source->copy());

    return addr;
}

byte* Thread::vpop(byte* addr) {
    /*  Run vpop instruction.
     *
     *  Vector always pops a copy of the object in a register.
     *  FIXME: make it possible to pop references.
     */
    int destination_register_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    Type* vector_operand = viua::operand::extract(addr)->resolve(this);
    int position_operand_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    /*  1) fetch vector,
     *  2) pop value at given index,
     *  3) put it in a register,
     */
    Type* ptr = static_cast<Vector*>(vector_operand)->pop(position_operand_index);
    if (destination_register_index) { place(destination_register_index, ptr); }

    return addr;
}

byte* Thread::vat(byte* addr) {
    /*  Run vat instruction.
     *
     *  Vector always returns a copy of the object in a register.
     *  FIXME: make it possible to pop references.
     */
    bool vector_operand_ref, destination_register_ref, position_operand_ref;
    int vector_operand_index, destination_register_index, position_operand_index;

    viua::cpu::util::extractIntegerOperand(addr, destination_register_ref, destination_register_index);
    viua::cpu::util::extractIntegerOperand(addr, vector_operand_ref, vector_operand_index);
    viua::cpu::util::extractIntegerOperand(addr, position_operand_ref, position_operand_index);

    if (vector_operand_ref) {
        vector_operand_index = static_cast<Integer*>(fetch(vector_operand_index))->value();
    }
    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }
    if (position_operand_ref) {
        position_operand_index = static_cast<Integer*>(fetch(position_operand_index))->value();
    }

    /*  1) fetch vector,
     *  2) pop value at given index,
     *  3) put it in a register,
     */
    Type* ptr = static_cast<Vector*>(fetch(vector_operand_index))->at(position_operand_index);
    place(destination_register_index, ptr);
    uregset->flag(destination_register_index, REFERENCE);

    return addr;
}

byte* Thread::vlen(byte* addr) {
    /*  Run vlen instruction.
     */
    bool vector_operand_ref, destination_register_ref;
    int vector_operand_index, destination_register_index;

    viua::cpu::util::extractIntegerOperand(addr, destination_register_ref, destination_register_index);
    viua::cpu::util::extractIntegerOperand(addr, vector_operand_ref, vector_operand_index);

    if (vector_operand_ref) {
        vector_operand_index = static_cast<Integer*>(fetch(vector_operand_index))->value();
    }
    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    place(destination_register_index, new Integer(static_cast<Vector*>(fetch(vector_operand_index))->len()));

    return addr;
}
