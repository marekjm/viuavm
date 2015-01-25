#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/object.h"
#include "../../types/integer.h"
#include "../../types/boolean.h"
#include "../../types/byte.h"
#include "../../support/pointer.h"
#include "../cpu.h"
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

    if (debug) {
        cout << (ref ? " @" : " ") << reg << endl;
    }

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

    if (debug) {
        cout << (a_ref ? " @" : " ") << a;
        cout << (b_ref ? " @" : " ") << b;
    }

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }
    if (b_ref) {
        b = static_cast<Integer*>(fetch(b))->value();
    }

    uregisters[b] = uregisters[a];    // copy pointer from first-operand register to second-operand register
    uregisters[a] = 0;               // zero first-operand register

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

    if (debug) {
        cout << (a_ref ? " @" : " ") << a;
        cout << (b_ref ? " @" : " ") << b;
    }

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

    if (debug) {
        cout << (a_ref ? " @" : " ") << a;
        cout << (b_ref ? " @" : " ") << b;
    }

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }
    if (b_ref) {
        b = static_cast<Integer*>(fetch(b))->value();
    }

    uregisters[b] = uregisters[a];    // copy pointer
    ureferences[b] = true;

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

    if (debug) {
        cout << (a_ref ? " @" : " ") << a;
        cout << (b_ref ? " @" : " ") << b;
    }

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }
    if (b_ref) {
        b = static_cast<Integer*>(fetch(b))->value();
    }

    Object* tmp = uregisters[a];
    uregisters[a] = uregisters[b];
    uregisters[b] = tmp;

    return addr;
}
byte* CPU::del(byte* addr) {
    return addr;
}
byte* CPU::isnull(byte* addr) {
    return addr;
}


byte* CPU::ret(byte* addr) {
    /*  Run iinc instruction.
     */
    bool ref = false;
    int regno;

    ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);

    regno = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (ref ? " @" : " ") << regno;
    }

    if (ref) {
        regno = static_cast<Integer*>(fetch(regno))->value();
    }

    if (debug) {
        if (ref) { cout << " -> " << regno; }
    }

    place(0, new Integer(static_cast<Integer*>(fetch(regno))->value()));

    return addr;
}

byte* CPU::frame(byte* addr) {
    /**
     */
    bool ref = false;
    int number;

    ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);

    number = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (ref ? " @" : " ") << number << endl;
    }

    if (ref) {
        number = static_cast<Integer*>(fetch(number))->value();
    }

    if (frame_new != 0) { throw "requested new frame while last one is unused"; }
    frame_new = new Frame(0, number);

    return addr;
}

byte* CPU::call(byte* addr) {
    /*  Run call instruction.
     */
    // save return address for frame
    byte* return_address = (addr + sizeof(bool) + 2*sizeof(int));
    if (debug) {
        cout << ": setting return address to bytecode " << (long)(return_address-bytecode);
    }
    if (frame_new == 0) {
        throw "function call without a frame: use `frame 0' if the function takes no parameters";
    }
    // adjust return address
    frame_new->return_address = return_address;
    // adjust register set
    uregisters = frame_new->registers;
    ureferences = frame_new->references;
    uregisters_size = frame_new->registers_size;

    // use frame for function call
    frames.push_back(frame_new);

    // and free the hook
    frame_new = 0;

    addr = bytecode + *(int*)addr;
    return addr;
}

byte* CPU::end(byte* addr) {
    /*  Run end instruction.
     */
    if (frames.size() == 0) {
        throw "no frame on stack: nothing to end";
    }
    addr = frames.back()->ret_address();
    if (debug) { cout << " -> return address: " << (long)(addr - bytecode); }
    delete frames.back();
    frames.pop_back();

    uregisters = registers;
    ureferences = references;
    uregisters_size = reg_count;

    return addr;
}

byte* CPU::jump(byte* addr) {
    /*  Run jump instruction.
     */
    if (debug) {
        cout << ' ' << *(int*)addr;
    }
    byte* target = bytecode+(*(int*)addr);
    if (target == addr) {
        throw "aborting: JUMP instruction pointing to itself";
    }
    return target;
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

    if (debug) {
        cout << dec << (regcond_ref ? " @" : " ") << regcond_num;
        cout << " " << addr_true  << "::0x" << hex << (long)(bytecode+addr_true) << dec;
        cout << " " << addr_false << "::0x" << hex << (long)(bytecode+addr_false);
    }


    if (regcond_ref) {
        if (debug) { cout << "resolving reference to condition register" << endl; }
        regcond_num = static_cast<Integer*>(fetch(regcond_num))->value();
    }

    bool result = fetch(regcond_num)->boolean();

    addr = bytecode + (result ? addr_true : addr_false);

    return addr;
}
