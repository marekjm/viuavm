#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/boolean.h>
#include <viua/types/byte.h>
#include <viua/types/boolean.h>
#include <viua/cpu/opex.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::lognot(byte* addr) {
    auto target = viua::operand::extract(addr);
    place(viua::operand::getRegisterIndexOrException(target.get(), this), new Boolean(not target->resolve(this)->boolean()));
    return addr;
}

byte* Thread::logand(byte* addr) {
    /*  Run ieq instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_index, second_operand_index, destination_register_index;

    viua::cpu::util::extractIntegerOperand(addr, destination_register_ref, destination_register_index);
    viua::cpu::util::extractIntegerOperand(addr, first_operand_ref, first_operand_index);
    viua::cpu::util::extractIntegerOperand(addr, second_operand_ref, second_operand_index);

    if (destination_register_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (first_operand_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (second_operand_ref) {
        second_operand_index = static_cast<Integer*>(fetch(second_operand_index))->value();
    }

    place(destination_register_index, new Boolean(fetch(first_operand_index)->boolean() and fetch(second_operand_index)->boolean()));

    return addr;
}

byte* Thread::logor(byte* addr) {
    /*  Run ieq instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_index, second_operand_index, destination_register_index;

    viua::cpu::util::extractIntegerOperand(addr, destination_register_ref, destination_register_index);
    viua::cpu::util::extractIntegerOperand(addr, first_operand_ref, first_operand_index);
    viua::cpu::util::extractIntegerOperand(addr, second_operand_ref, second_operand_index);

    if (destination_register_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (first_operand_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (second_operand_ref) {
        second_operand_index = static_cast<Integer*>(fetch(second_operand_index))->value();
    }

    place(destination_register_index, new Boolean(fetch(first_operand_index)->boolean() or fetch(second_operand_index)->boolean()));

    return addr;
}
