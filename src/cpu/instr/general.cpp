#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/object.h"
#include "../../types/integer.h"
#include "../../types/boolean.h"
#include "../../types/byte.h"
#include "../../support/pointer.h"
#include "../cpu.h"
#include "../registerset.h"
using namespace std;


byte* CPU::echo(byte* addr) {
    /*  Run echo instruction.
     */
    bool ref = false;
    int reg;

    ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);

    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    cout << fetch(reg)->str();

    return addr;
}

byte* CPU::print(byte* addr) {
    /*  Run print instruction.
     */
    addr = echo(addr);
    cout << '\n';
    return addr;
}


byte* CPU::move(byte* addr) {
    /** Run move instruction.
     *  Move an object from one register into another.
     */
    int a, b;
    bool a_ref = false, b_ref = false;

    a_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    a = *((int*)addr);
    pointer::inc<int, byte>(addr);

    b_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    b = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }
    if (b_ref) {
        b = static_cast<Integer*>(fetch(b))->value();
    }

    uregset->move(a, b);
    return addr;
}
byte* CPU::copy(byte* addr) {
    /** Run move instruction.
     *  Copy an object from one register into another.
     */
    int a, b;
    bool a_ref = false, b_ref = false;

    a_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    a = *((int*)addr);
    pointer::inc<int, byte>(addr);

    b_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    b = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }
    if (b_ref) {
        b = static_cast<Integer*>(fetch(b))->value();
    }

    place(b, fetch(a)->copy());

    return addr;
}
byte* CPU::ref(byte* addr) {
    /** Run ref instruction.
     *  Create a reference (implementation detail: copy a pointer) of an object in one register in
     *  another register.
     */
    int a, b;
    bool a_ref = false, b_ref = false;

    a_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    a = *((int*)addr);
    pointer::inc<int, byte>(addr);

    b_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    b = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }
    if (b_ref) {
        b = static_cast<Integer*>(fetch(b))->value();
    }

    uregset->set(b, uregset->get(a));
    uregset->flag(b, REFERENCE);

    return addr;
}
byte* CPU::swap(byte* addr) {
    /** Run swap instruction.
     *  Swaps two objects in registers.
     */
    int a, b;
    bool a_ref = false, b_ref = false;

    a_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    a = *((int*)addr);
    pointer::inc<int, byte>(addr);

    b_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    b = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }
    if (b_ref) {
        b = static_cast<Integer*>(fetch(b))->value();
    }

    uregset->swap(a, b);
    return addr;
}
byte* CPU::free(byte* addr) {
    /** Run free instruction.
     */
    int a;
    bool a_ref = false;

    a_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    a = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }

    uregset->free(a);

    return addr;
}
byte* CPU::empty(byte* addr) {
    /** Run empty instruction.
     */
    int a;
    bool a_ref = false;

    a_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    a = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }

    uregset->empty(a);

    return addr;
}
byte* CPU::isnull(byte* addr) {
    /** Run isnull instruction.
     *
     * Example:
     *
     *      isnull A, B
     *
     * the above means: "check if A is null and store the information in B".
     */
    int a, b;
    bool a_ref = false, b_ref = false;

    a_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    a = *((int*)addr);
    pointer::inc<int, byte>(addr);

    b_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    b = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }
    if (b_ref) {
        b = static_cast<Integer*>(fetch(b))->value();
    }

    place(b, new Boolean(uregset->at(a) == 0));

    return addr;
}

byte* CPU::ress(byte* addr) {
    /*  Run ress instruction.
     */
    int to_register_set = 0;
    to_register_set = *(int*)addr;
    pointer::inc<int, byte>(addr);

    switch (to_register_set) {
        case 0:
            uregset = regset;
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
            throw "illegal register set ID in ress instruction";
    }

    return addr;
}

byte* CPU::tmpri(byte* addr) {
    /** Run tmpri instruction.
     */
    int a;
    bool a_ref = false;

    a_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    a = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }

    if (tmp != 0) {
        cout << "warning: CPU: storing in non-empty temporary register: memory has been leaked" << endl;
    }
    tmp = uregset->get(a)->copy();

    return addr;
}
byte* CPU::tmpro(byte* addr) {
    /** Run tmpro instruction.
     */
    int a;
    bool a_ref = false;

    a_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    a = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }

    if (uregset->at(a) != 0) {
        if (errors) {
            cerr << "warning: CPU: droping from temporary into non-empty register: possible references loss and register corruption" << endl;
        }
        uregset->free(a);
    }
    uregset->set(a, tmp);
    tmp = 0;

    return addr;
}

byte* CPU::frame(byte* addr) {
    /** Create new frame for function calls.
     */
    int arguments, local_registers;
    bool arguments_ref = false, local_registers_ref = false;

    arguments_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    arguments = *((int*)addr);
    pointer::inc<int, byte>(addr);

    local_registers_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    local_registers = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (arguments_ref) {
        arguments = static_cast<Integer*>(fetch(arguments))->value();
    }
    if (local_registers_ref) {
        local_registers = static_cast<Integer*>(fetch(local_registers))->value();
    }

    requestNewFrame(arguments, local_registers);
    return addr;
}

byte* CPU::param(byte* addr) {
    /** Run param instruction.
     */
    int a, b;
    bool a_ref = false, b_ref = false;

    a_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    a = *((int*)addr);
    pointer::inc<int, byte>(addr);

    b_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    b = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }
    if (b_ref) {
        b = static_cast<Integer*>(fetch(b))->value();
    }

    if (unsigned(a) >= frame_new->args->size()) { throw "parameter register index out of bounds (greater than arguments set size) while adding parameter"; }
    frame_new->args->set(a, fetch(b)->copy());
    frame_new->args->clear(a);

    return addr;
}

byte* CPU::paref(byte* addr) {
    /** Run paref instruction.
     */
    int a, b;
    bool a_ref = false, b_ref = false;

    a_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    a = *((int*)addr);
    pointer::inc<int, byte>(addr);

    b_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    b = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }
    if (b_ref) {
        b = static_cast<Integer*>(fetch(b))->value();
    }

    if (unsigned(a) >= frame_new->args->size()) { throw "parameter register index out of bounds (greater than arguments set size) while adding parameter"; }
    frame_new->args->set(a, fetch(b));
    frame_new->args->flag(a, REFERENCE);

    return addr;
}

byte* CPU::arg(byte* addr) {
    /** Run arg instruction.
     */
    int a, b;
    bool a_ref = false, b_ref = false;

    a_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    a = *((int*)addr);
    pointer::inc<int, byte>(addr);

    b_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    b = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }
    if (b_ref) {
        b = static_cast<Integer*>(fetch(b))->value();
    }

    if (unsigned(a) >= frames.back()->args->size()) {
        ostringstream oss;
        oss << "invalid read: read from argument register out of bounds: " << a;
        throw oss.str().c_str();
    }

    if (frames.back()->args->isflagged(a, REFERENCE)) {
        uregset->set(b, frames.back()->args->get(a));
    } else {
        uregset->set(b, frames.back()->args->get(a)->copy());
    }
    uregset->setmask(b, frames.back()->args->getmask(a));  // set correct mask

    return addr;
}

byte* CPU::call(byte* addr) {
    /*  Run call instruction.
     */
    string call_name = string(addr);
    byte* call_address = bytecode+function_addresses.at(call_name);
    addr += call_name.size();

    // save return address for frame
    byte* return_address = (addr + sizeof(bool) + sizeof(int));

    if (frame_new == 0) {
        throw "function call without a frame: use `frame 0' in source code if the function takes no parameters";
    }
    // set function name and return address
    frame_new->function_name = call_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = *(bool*)addr;
    pointer::inc<bool, byte>(addr);
    frame_new->place_return_value_in = *(int*)addr;

    pushFrame();

    return call_address;
}

byte* CPU::end(byte* addr) {
    /*  Run end instruction.
     */
    if (frames.size() == 0) {
        throw "no frame on stack: nothing to end";
    }
    addr = frames.back()->ret_address();

    Object* returned = 0;
    bool returned_is_reference = false;
    int return_value_register = frames.back()->place_return_value_in;
    bool resolve_return_value_register = frames.back()->resolve_return_value_register;
    if (return_value_register != 0) {
        // we check in 0. register because it's reserved for return values
        if (uregset->at(0) == 0) {
            throw "return value requested by frame but function did not set return register";
        }
        if (uregset->isflagged(0, REFERENCE)) {
            returned = uregset->get(0);
            returned_is_reference = true;
        } else {
            returned = uregset->get(0)->copy();
        }
    }

    dropFrame();

    // place return value
    if (returned and frames.size() > 0) {
        if (resolve_return_value_register) {
            return_value_register = static_cast<Integer*>(fetch(return_value_register))->value();
        }
        place(return_value_register, returned);
        if (returned_is_reference) {
            uregset->flag(return_value_register, REFERENCE);
        }
    }

    return addr;
}

byte* CPU::jump(byte* addr) {
    /*  Run jump instruction.
     */
    byte* target = bytecode+(*(int*)addr);
    if (target == addr) {
        throw "aborting: JUMP instruction pointing to itself";
    }
    return target;
}


byte* CPU::excall(byte* addr) {
    /** Run excall instruction.
     */
    string call_name = string(addr);
    addr += call_name.size();

    // save return address for frame
    byte* return_address = (addr + sizeof(bool) + sizeof(int));

    if (frame_new == 0) {
        throw "external function call without a frame: use `frame 0' in source code if the function takes no parameters";
    }
    // set function name and return address
    frame_new->function_name = call_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = *(bool*)addr;
    pointer::inc<bool, byte>(addr);
    frame_new->place_return_value_in = *(int*)addr;

    Frame* frame = frame_new;

    pushFrame();

    if (external_functions.count(call_name) == 0) {
        throw ("call to unregistered external function: " + call_name);
    }

    /* FIXME: second parameter should be a pointer to static registers or
     *        0 if function does not have static registers registered
     * FIXME: should external functions always have static registers allocated?
     */
    Object* returned = 0;
    externalFunction* callback = external_functions.at(call_name);
    (*callback)(frame, 0, regset);

    // FIXME: woohoo! segfault!
    bool returned_is_reference = false;
    int return_value_register = frames.back()->place_return_value_in;
    bool resolve_return_value_register = frames.back()->resolve_return_value_register;
    if (return_value_register != 0) {
        // we check in 0. register because it's reserved for return values
        if (uregset->at(0) == 0) {
            throw "return value requested by frame but external function did not set return register";
        }
        if (uregset->isflagged(0, REFERENCE)) {
            returned = uregset->get(0);
            returned_is_reference = true;
        } else {
            returned = uregset->get(0)->copy();
        }
    }

    dropFrame();

    // place return value
    if (returned and frames.size() > 0) {
        if (resolve_return_value_register) {
            return_value_register = static_cast<Integer*>(fetch(return_value_register))->value();
        }
        place(return_value_register, returned);
        if (returned_is_reference) {
            uregset->flag(return_value_register, REFERENCE);
        }
    }

    return return_address;
}


byte* CPU::branch(byte* addr) {
    /*  Run branch instruction.
     */
    bool regcond_ref;
    int regcond_num;


    regcond_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);

    regcond_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    int addr_true = *((int*)addr);
    pointer::inc<int, byte>(addr);

    int addr_false = *((int*)addr);
    pointer::inc<int, byte>(addr);


    if (regcond_ref) {
        regcond_num = static_cast<Integer*>(fetch(regcond_num))->value();
    }

    bool result = fetch(regcond_num)->boolean();

    addr = bytecode + (result ? addr_true : addr_false);

    return addr;
}
