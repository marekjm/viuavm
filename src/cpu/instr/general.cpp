#include <dlfcn.h>
#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/type.h"
#include "../../types/integer.h"
#include "../../types/boolean.h"
#include "../../types/byte.h"
#include "../../support/pointer.h"
#include "../../include/module.h"
#include "../../exceptions.h"
#include "../cpu.h"
#include "../registerset.h"
using namespace std;


byte* CPU::echo(byte* addr) {
    /*  Run echo instruction.
     */
    bool ref = false;
    int operand_index;

    ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);

    operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (ref) {
        operand_index = static_cast<Integer*>(fetch(operand_index))->value();
    }

    cout << fetch(operand_index)->str();

    return addr;
}

byte* CPU::print(byte* addr) {
    /*  Run print instruction.
     */
    addr = echo(addr);
    cout << '\n';
    return addr;
}


byte* CPU::jump(byte* addr) {
    /*  Run jump instruction.
     */
    byte* target = bytecode+(*(int*)addr);
    if (target == addr) {
        throw new Exception("aborting: JUMP instruction pointing to itself");
    }
    return target;
}

byte* CPU::tryframe(byte* addr) {
    /** Create new special frame for try blocks.
     */
    if (try_frame_new != 0) {
        throw "new block frame requested while last one is unused";
    }
    try_frame_new = new TryFrame();
    return addr;
}

byte* CPU::vmcatch(byte* addr) {
    /** Run catch instruction.
     */
    string type_name = string(addr);
    addr += (type_name.size()+1);

    string catcher_block_name = string(addr);
    addr += (catcher_block_name.size()+1);

    try_frame_new->catchers[type_name] = new Catcher(type_name, catcher_block_name, (bytecode+block_addresses.at(catcher_block_name)));

    return addr;
}

byte* CPU::pull(byte* addr) {
    /** Run pull instruction.
     */
    int destination_register_index;
    bool destination_register_ref = false;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    if (caught == 0) {
        throw new Exception("no caught object to pull");
    }
    uregset->set(destination_register_index, caught);
    caught = 0;

    return addr;
}

byte* CPU::vmtry(byte* addr) {
    /*  Run try instruction.
     */
    string block_name = string(addr);
    byte* block_address = bytecode+block_addresses.at(block_name);

    try_frame_new->return_address = (addr+block_name.size());
    try_frame_new->associated_frame = frames.back();
    try_frame_new->block_name = block_name;

    tryframes.push_back(try_frame_new);
    try_frame_new = 0;

    return block_address;
}

byte* CPU::vmthrow(byte* addr) {
    /** Run throw instruction.
     */
    int source_register_index;
    bool source_register_ref = false;

    source_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    source_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (source_register_ref) {
        source_register_index = static_cast<Integer*>(fetch(source_register_index))->value();
    }

    if (unsigned(source_register_index) >= uregset->size()) {
        ostringstream oss;
        oss << "invalid read: register out of bounds: " <<source_register_index;
        throw new Exception(oss.str());
    }
    if (uregset->at(source_register_index) == 0) {
        ostringstream oss;
        oss << "invalid throw: register " << source_register_index << " is empty";
        throw new Exception(oss.str());
    }

    uregset->setmask(source_register_index, KEEP);  // set correct mask
    thrown = uregset->get(source_register_index);

    return addr;
}

byte* CPU::leave(byte* addr) {
    /*  Run leave instruction.
     */
    if (tryframes.size() == 0) {
        throw Exception("bad leave: no block has been entered");
    }
    addr = tryframes.back()->return_address;
    delete tryframes.back();
    tryframes.pop_back();
    return addr;
}

byte* CPU::eximport(byte* addr) {
    /** Run excall instruction.
     */
    string module = string(addr);
    addr += module.size();

    string path = ("./" + module + ".so");
    void* handle = dlopen(path.c_str(), RTLD_LAZY);

    ostringstream oss;
    for (unsigned i = 0; (i < VIUAPATH.size()) and (handle == 0); ++i) {
        oss.str("");
        oss << VIUAPATH[i] << '/' << module << ".so";
        path = oss.str();
        if (path[0] == '~') {
            oss.str("");
            oss << getenv("HOME") << path.substr(1);
            path = oss.str();
        }
        handle = dlopen(path.c_str(), RTLD_LAZY);
    }

    if (handle == 0) {
        throw new Exception("LinkException", ("failed to link library: " + module));
    }

    ExternalFunctionSpec* (*exports)() = 0;
    if ((exports = (ExternalFunctionSpec*(*)())dlsym(handle, "exports")) == 0) {
        throw new Exception("failed to extract interface from module: " + module);
    }

    ExternalFunctionSpec* exported = (*exports)();

    unsigned i = 0;
    while (exported[i].name != NULL) {
        string namespaced_name = (module + '.' + exported[i].name);
        registerExternalFunction(namespaced_name, exported[i].fpointer);
        ++i;
    }

    return addr;
}
byte* CPU::excall(byte* addr) {
    /** Run excall instruction.
     */
    string call_name = string(addr);
    addr += (call_name.size()+1);

    // save return address for frame
    byte* return_address = (addr + sizeof(bool) + sizeof(int));

    if (frame_new == 0) {
        throw new Exception("external function call without a frame: use `frame 0' in source code if the function takes no parameters");
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
        throw new Exception("call to unregistered external function: " + call_name);
    }

    /* FIXME: second parameter should be a pointer to static registers or
     *        0 if function does not have static registers registered
     * FIXME: should external functions always have static registers allocated?
     */
    ExternalFunction* callback = external_functions.at(call_name);
    (*callback)(frame, 0, regset);

    // FIXME: woohoo! segfault!
    Type* returned = 0;
    bool returned_is_reference = false;
    int return_value_register = frames.back()->place_return_value_in;
    bool resolve_return_value_register = frames.back()->resolve_return_value_register;
    if (return_value_register != 0) {
        // we check in 0. register because it's reserved for return values
        if (uregset->at(0) == 0) {
            throw new Exception("return value requested by frame but external function did not set return register");
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
    bool condition_object_ref;
    int condition_object_index;

    condition_object_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);

    condition_object_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    int addr_true = *((int*)addr);
    pointer::inc<int, byte>(addr);

    int addr_false = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (condition_object_ref) {
        condition_object_index = static_cast<Integer*>(fetch(condition_object_index))->value();
    }

    bool result = fetch(condition_object_index)->boolean();

    addr = bytecode + (result ? addr_true : addr_false);

    return addr;
}
