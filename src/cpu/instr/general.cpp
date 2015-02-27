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
byte* CPU::free(byte* addr) {
    /** Run free instruction.
     */
    int a;
    bool a_ref = false;

    a_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    a = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (a_ref ? " @" : " ") << a;
    }

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }

    if (uregisters[a]) {
        // FIXME: if it is a reference register all references should be cleared
        // to avoid errors and
        // to make it possible to detect deletion with "isnull" instruction
        delete uregisters[a];
        uregisters[a] = 0;
    }
    ureferences[a] = false;

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

    if (debug) {
        cout << (a_ref ? " @" : " ") << a;
    }

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }

    uregisters[a] = 0;
    ureferences[a] = false;

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

    place(b, new Boolean(uregisters[a] == 0));

    return addr;
}

byte* CPU::ress(byte* addr) {
    /*  Run ress instruction.
     */
    int to_register_set = 0;
    to_register_set = *(int*)addr;
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << ' ';
        switch (to_register_set) {
            case 0:
                cout << "global";
                break;
            case 1:
                cout << "local";
                break;
            case 2:
                cout << "static (WIP)";
                break;
            case 3:
                cout << "temp (TODO)";
                break;
            default:
                cout << "illegal";
        }
    }

    switch (to_register_set) {
        case 0:
            uregisters = registers;
            ureferences = references;
            uregisters_size = reg_count;
            break;
        case 1:
            uregisters = frames.back()->registers;
            ureferences = frames.back()->references;
            uregisters_size = frames.back()->registers_size;
            break;
        case 2:
            ensureStaticRegisters(frames.back()->function_name);
            tie(uregisters, ureferences, uregisters_size) = static_registers.at(frames.back()->function_name);
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

    if (debug) {
        cout << (a_ref ? " @" : " ") << a;
    }

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }

    if (tmp != 0) {
        cout << "warning: CPU: storing in non-empty temporary register: memory has been leaked" << endl;
    }
    tmp = uregisters[a]->copy();

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

    if (debug) {
        cout << (a_ref ? " @" : " ") << a;
    }

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }

    if (uregisters[a] != 0) {
        if (errors) {
            cerr << "warning: CPU: droping from temporary into non-empty register: possible references loss" << endl;
        }
        delete uregisters[a];
    }
    uregisters[a] = tmp;
    ureferences[a] = false;
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

    if (debug) {
        cout << (arguments_ref ? " @" : " ") << arguments;
        cout << (local_registers_ref ? " @" : " ") << local_registers;
    }

    if (arguments_ref) {
        arguments = static_cast<Integer*>(fetch(arguments))->value();
    }
    if (local_registers_ref) {
        local_registers = static_cast<Integer*>(fetch(local_registers))->value();
    }

    if (frame_new != 0) { throw "requested new frame while last one is unused"; }
    frame_new = new Frame(0, arguments, local_registers);

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

    if (a >= frame_new->arguments_size) { throw "parameter register index out of bounds (greater than arguments set size) while adding parameter"; }
    frame_new->arguments[a] = fetch(b)->copy();

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

    if (a >= frame_new->arguments_size) { throw "parameter register index out of bounds (greater than arguments set size) while adding parameter"; }
    frame_new->arguments[a] = uregisters[b];
    frame_new->argreferences[a] = true;

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

    uregisters[b] = frames.back()->arguments[a];    // move pointer from first-operand register to second-operand register
    frames.back()->arguments[a] = 0;                // zero the pointer to avoid double free
    ureferences[b] = frames.back()->argreferences[a];  // set reference status

    if (debug and uregisters[b] != 0) {
        cout << " (= " << (ureferences[b] ? "&" : "") << uregisters[b]->str() << ')';
    }

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

    if (debug) {
        cout << " '" << call_name << "' " << (long)(call_address-bytecode);
        cout << ": setting return address to bytecode " << (long)(return_address-bytecode);
    }
    if (frame_new == 0) {
        throw "function call without a frame: use `frame 0' in source code if the function takes no parameters";
    }
    // set function name and return address
    frame_new->function_name = call_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = *(bool*)addr;
    pointer::inc<bool, byte>(addr);
    frame_new->place_return_value_in = *(int*)addr;
    if (debug) {
        if (frame_new->place_return_value_in == 0) {
            cout << " (return value will be discarded)";
        } else {
            cout << " (return value will be placed in: " << (frame_new->resolve_return_value_register ? "@" : "") << frame_new->place_return_value_in << ')';
        }
    }

    // adjust register set
    uregisters = frame_new->registers;
    ureferences = frame_new->references;
    uregisters_size = frame_new->registers_size;

    // use frame for function call
    frames.push_back(frame_new);

    // and free the hook
    frame_new = 0;

    return call_address;
}

byte* CPU::end(byte* addr) {
    /*  Run end instruction.
     */
    if (frames.size() == 0) {
        throw "no frame on stack: nothing to end";
    }
    addr = frames.back()->ret_address();
    if (debug) {
        cout << " -> return address: " << (long)(addr - bytecode);
        cout << " (from function: " << frames.back()->function_name << ')';
    }

    Object* returned = 0;
    bool returned_is_reference = false;
    int return_value_register = frames.back()->place_return_value_in;
    bool resolve_return_value_register = frames.back()->resolve_return_value_register;
    if (return_value_register != 0) {
        // we check in 0. register because it's reserved for return values
        if (uregisters[0] == 0) {
            throw "return value requested by frame but function did not set return register";
        }
        if (ureferences[0]) {
            returned = uregisters[0];
            returned_is_reference = true;
        } else {
            returned = uregisters[0]->copy();
        }
    }

    // delete and remove top frame
    delete frames.back();
    frames.pop_back();

    // adjust registers
    if (frames.size()) {
        uregisters = frames.back()->registers;
        ureferences = frames.back()->references;
        uregisters_size = frames.back()->registers_size;
    } else {
        uregisters = registers;
        ureferences = references;
        uregisters_size = reg_count;
    }

    // place return value
    if (returned and frames.size() > 0) {
        if (resolve_return_value_register) {
            return_value_register = static_cast<Integer*>(fetch(return_value_register))->value();
        }
        place(return_value_register, returned);
        ureferences[return_value_register] = returned_is_reference;
    }

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
