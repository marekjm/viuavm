#include <iostream>
#include <viua/types/pointer.h>
#include <viua/types/boolean.h>
#include <viua/types/reference.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::opmove(byte* addr) {
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    int source = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    uregset->move(source, target);

    return addr;
}
byte* Thread::opcopy(byte* addr) {
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    place(target, viua::operand::extract(addr)->resolve(this)->copy());

    return addr;
}
byte* Thread::opref(byte* addr) {
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
byte* Thread::opptr(byte* addr) {
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    place(target, viua::operand::extract(addr)->resolve(this)->pointer());

    return addr;
}
byte* Thread::opswap(byte* addr) {
    int first = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    int second = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    uregset->swap(first, second);

    return addr;
}
byte* Thread::opdelete(byte* addr) {
    uregset->free(viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this));
    return addr;
}
byte* Thread::opempty(byte* addr) {
    /** Run empty instruction.
     */
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    Type* object = uregset->get(target);
    if (Reference* rf = dynamic_cast<Reference*>(object)) {
        delete rf;
    }
    uregset->empty(target);

    return addr;
}
byte* Thread::opisnull(byte* addr) {
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    int source = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    place(target, new Boolean(uregset->at(source) == nullptr));

    return addr;
}

byte* Thread::opress(byte* addr) {
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

byte* Thread::optmpri(byte* addr) {
    // FIXME: mutex (VERY IMPORTANT!!!)
    if (cpu->tmp != nullptr) {
        cout << "warning: CPU: storing in non-empty temporary register: memory has been leaked" << endl;
    }
    cpu->tmp = viua::operand::extract(addr)->resolve(this)->copy();

    return addr;
}
byte* Thread::optmpro(byte* addr) {
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    if (uregset->at(target) != nullptr) {
        if (cpu->errors) {
            cerr << "warning: CPU: droping from temporary into non-empty register: possible references loss and register corruption" << endl;
        }
        uregset->free(target);
    }
    // FIXME: mutex (VERY IMPORTANT!!!)
    uregset->set(target, cpu->tmp);
    cpu->tmp = nullptr;

    return addr;
}
