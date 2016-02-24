#include <viua/types/integer.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::vmtry(byte* addr) {
    /** Create new special frame for try blocks.
     */
    if (try_frame_new != nullptr) {
        throw "new block frame requested while last one is unused";
    }
    try_frame_new = new TryFrame();
    return addr;
}

byte* Thread::vmcatch(byte* addr) {
    /** Run catch instruction.
     */
    string type_name = viua::operand::extractString(addr);
    string catcher_block_name = viua::operand::extractString(addr);

    bool block_found = (cpu->block_addresses.count(catcher_block_name) or cpu->linked_blocks.count(catcher_block_name));
    if (not block_found) {
        throw new Exception("registering undefined handler block: " + catcher_block_name);
    }

    byte* block_address = nullptr;
    if (cpu->block_addresses.count(catcher_block_name)) {
        block_address = cpu->bytecode+cpu->block_addresses.at(catcher_block_name);
    } else {
        block_address = cpu->linked_blocks.at(catcher_block_name).second;
    }

    try_frame_new->catchers[type_name] = new Catcher(type_name, catcher_block_name, block_address);

    return addr;
}

byte* Thread::pull(byte* addr) {
    /** Run pull instruction.
     */
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    if (caught == nullptr) {
        throw new Exception("no caught object to pull");
    }
    uregset->set(target, caught);
    caught = nullptr;

    return addr;
}

byte* Thread::vmenter(byte* addr) {
    /*  Run enter instruction.
     */
    string block_name = viua::operand::extractString(addr);

    bool block_found = (cpu->block_addresses.count(block_name) or cpu->linked_blocks.count(block_name));
    if (not block_found) {
        throw new Exception("cannot enter undefined block: " + block_name);
    }

    byte* block_address = nullptr;
    if (cpu->block_addresses.count(block_name)) {
        block_address = cpu->bytecode+cpu->block_addresses.at(block_name);
        jump_base = cpu->bytecode;
    } else {
        block_address = cpu->linked_blocks.at(block_name).second;
        jump_base = cpu->linked_modules.at(cpu->linked_blocks.at(block_name).first).second;
    }

    try_frame_new->return_address = addr; // address has already been adjusted by extractString()
    try_frame_new->associated_frame = frames.back();
    try_frame_new->block_name = block_name;

    tryframes.push_back(try_frame_new);
    try_frame_new = nullptr;

    return block_address;
}

byte* Thread::vmthrow(byte* addr) {
    /** Run throw instruction.
     */
    int source = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    if (unsigned(source) >= uregset->size()) {
        ostringstream oss;
        oss << "invalid read: register out of bounds: " << source;
        throw new Exception(oss.str());
    }
    if (uregset->at(source) == nullptr) {
        ostringstream oss;
        oss << "invalid throw: register " << source << " is empty";
        throw new Exception(oss.str());
    }

    thrown = uregset->get(source);
    uregset->empty(source);

    return addr;
}

byte* Thread::leave(byte* addr) {
    /*  Run leave instruction.
     */
    if (tryframes.size() == 0) {
        throw new Exception("bad leave: no block has been entered");
    }
    addr = tryframes.back()->return_address;
    delete tryframes.back();
    tryframes.pop_back();

    if (frames.size() > 0) {
        if (cpu->function_addresses.count(frames.back()->function_name)) {
            jump_base = cpu->bytecode;
        } else {
            jump_base = cpu->linked_modules.at(cpu->linked_functions.at(frames.back()->function_name).first).second;
        }
    }
    return addr;
}
