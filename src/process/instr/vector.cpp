#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/assert.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/vector.h>
#include <viua/cpu/opex.h>
#include <viua/cpu/registerset.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Process::opvec(byte* addr) {
    place(viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this), new Vector());

    return addr;
}

byte* Process::opvinsert(byte* addr) {
    /*  Run vinsert instruction.
     */
    Type* vector_operand = viua::operand::extract(addr)->resolve(this);
    int object_operand_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    int position_operand_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

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
    int source = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    viua::assertions::assert_implements<Vector>(target, "Vector");
    static_cast<Vector*>(target)->push(pop(source));

    return addr;
}

byte* Process::opvpop(byte* addr) {
    /*  Run vpop instruction.
     */
    int destination_register_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    Type* vector_operand = viua::operand::extract(addr)->resolve(this);
    int position_operand_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

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
    int destination_register_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    Type* vector_operand = viua::operand::extract(addr)->resolve(this);
    int position_operand_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    viua::assertions::assert_implements<Vector>(vector_operand, "Vector");
    Type* ptr = static_cast<Vector*>(vector_operand)->at(position_operand_index);
    place(destination_register_index, ptr->copy());

    return addr;
}

byte* Process::opvlen(byte* addr) {
    /*  Run vlen instruction.
     */
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    Type* source = viua::operand::extract(addr)->resolve(this);

    viua::assertions::assert_implements<Vector>(source, "Vector");
    place(target, new Integer(static_cast<Vector*>(source)->len()));

    return addr;
}
