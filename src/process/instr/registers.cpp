#include <iostream>
#include <viua/types/pointer.h>
#include <viua/types/boolean.h>
#include <viua/types/reference.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Process::opmove(byte* addr) {
    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    unsigned source = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    uregset->move(source, target);

    return addr;
}
byte* Process::opcopy(byte* addr) {
    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    place(target, viua::operand::extract(addr)->resolve(this)->copy());

    return addr;
}
byte* Process::opptr(byte* addr) {
    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    place(target, viua::operand::extract(addr)->resolve(this)->pointer());

    return addr;
}
byte* Process::opswap(byte* addr) {
    unsigned first = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    unsigned second = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    uregset->swap(first, second);

    return addr;
}
byte* Process::opdelete(byte* addr) {
    uregset->free(viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this));
    return addr;
}
byte* Process::opempty(byte* addr) {
    /** Run empty instruction.
     */
    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    Type* object = uregset->get(target);
    if (Reference* rf = dynamic_cast<Reference*>(object)) {
        delete rf;
    }
    uregset->empty(target);

    return addr;
}
byte* Process::opisnull(byte* addr) {
    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    unsigned source = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    place(target, new Boolean(uregset->at(source) == nullptr));

    return addr;
}

byte* Process::opress(byte* addr) {
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

byte* Process::optmpri(byte* addr) {
    // FIXME: mutex (VERY IMPORTANT!!!)
    if (cpu->tmp != nullptr) {
        cout << "warning: CPU: storing in non-empty temporary register: memory has been leaked" << endl;
    }
    cpu->tmp = viua::operand::extract(addr)->resolve(this)->copy();

    return addr;
}
byte* Process::optmpro(byte* addr) {
    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

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
