#include <iostream>
#include <viua/types/boolean.h>
#include <viua/types/reference.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::move(byte* addr) {
    /** Run move instruction.
     *  Move an object from one register into another.
     */
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    int source = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    uregset->move(source, target);

    return addr;
}
byte* Thread::copy(byte* addr) {
    /** Run copy instruction.
     *  Copy an object from one register into another.
     */
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    place(target, viua::operand::extract(addr)->resolve(this)->copy());

    return addr;
}
byte* Thread::ref(byte* addr) {
    /** Run ref instruction.
     *  Create object_operand_index reference (implementation detail: copy object_operand_index pointer) of an object in one register in
     *  another register.
     */
    int object_operand_index, destination_register_index;
    bool object_operand_ref = false, destination_register_ref = false;

    viua::cpu::util::extractIntegerOperand(addr, destination_register_ref, destination_register_index);
    viua::cpu::util::extractIntegerOperand(addr, object_operand_ref, object_operand_index);

    if (object_operand_ref) {
        object_operand_index = static_cast<Integer*>(fetch(object_operand_index))->value();
    }
    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    Type* object = uregset->at(object_operand_index);
    Reference* rf = nullptr;
    if (dynamic_cast<Reference*>(object)) {
        //cout << "thou shalt not reference references!" << endl;
        rf = static_cast<Reference*>(object)->copy();
        uregset->set(destination_register_index, rf);
    } else {
        rf = new Reference(object);
        uregset->empty(object_operand_index);
        uregset->set(object_operand_index, rf);
        uregset->set(destination_register_index, rf->copy());
    }

    return addr;
}
byte* Thread::swap(byte* addr) {
    int first = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    int second = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    uregset->swap(first, second);

    return addr;
}
byte* Thread::free(byte* addr) {
    uregset->free(viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this));
    return addr;
}
byte* Thread::empty(byte* addr) {
    /** Run empty instruction.
     */
    int target_register_index;
    bool target_register_ref = false;

    viua::cpu::util::extractIntegerOperand(addr, target_register_ref, target_register_index);

    if (target_register_ref) {
        target_register_index = static_cast<Integer*>(fetch(target_register_index))->value();
    }

    Type* object = uregset->get(target_register_index);
    if (Reference* rf = dynamic_cast<Reference*>(object)) {
        delete rf;
    }
    uregset->empty(target_register_index);

    return addr;
}
byte* Thread::isnull(byte* addr) {
    /** Run isnull instruction.
     *
     * Example:
     *
     *      isnull checked_register_index, B
     *
     * the above means: "check if checked_register_index is null and store the information in B".
     */
    int checked_register_index, destination_register_index;
    bool checked_register_ref = false, destination_register_ref = false;

    viua::cpu::util::extractIntegerOperand(addr, destination_register_ref, destination_register_index);
    viua::cpu::util::extractIntegerOperand(addr, checked_register_ref, checked_register_index);

    if (checked_register_ref) {
        checked_register_index = static_cast<Integer*>(fetch(checked_register_index))->value();
    }
    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    place(destination_register_index, new Boolean(uregset->at(checked_register_index) == nullptr));

    return addr;
}

byte* Thread::ress(byte* addr) {
    /*  Run ress instruction.
     */
    int to_register_set = 0;
    viua::cpu::util::extractOperand<decltype(to_register_set)>(addr, to_register_set);

    switch (to_register_set) {
        case 0:
            uregset = cpu->regset;
            break;
        case 1:
            uregset = frames.back()->regset;
            break;
        case 2:
            ensureStaticRegisters(frames.back()->function_name);
            uregset = static_registers.at(frames.back()->function_name);
            break;
        case 3:
            // TODO: switching to temporary registers
        default:
            throw new Exception("illegal register set ID in ress instruction");
    }

    return addr;
}

byte* Thread::tmpri(byte* addr) {
    /** Run tmpri instruction.
     */
    int object_operand_index;
    bool object_operand_ref = false;

    viua::cpu::util::extractIntegerOperand(addr, object_operand_ref, object_operand_index);

    if (object_operand_ref) {
        object_operand_index = static_cast<Integer*>(fetch(object_operand_index))->value();
    }

    // FIXME: mutex (VERY IMPORTANT!!!)
    if (cpu->tmp != nullptr) {
        cout << "warning: CPU: storing in non-empty temporary register: memory has been leaked" << endl;
    }
    cpu->tmp = uregset->get(object_operand_index)->copy();

    return addr;
}
byte* Thread::tmpro(byte* addr) {
    /** Run tmpro instruction.
     */
    int destination_register_index;
    bool destination_register_ref = false;

    viua::cpu::util::extractIntegerOperand(addr, destination_register_ref, destination_register_index);

    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    if (uregset->at(destination_register_index) != nullptr) {
        if (cpu->errors) {
            cerr << "warning: CPU: droping from temporary into non-empty register: possible references loss and register corruption" << endl;
        }
        uregset->free(destination_register_index);
    }
    // FIXME: mutex (VERY IMPORTANT!!!)
    uregset->set(destination_register_index, cpu->tmp);
    cpu->tmp = nullptr;

    return addr;
}
