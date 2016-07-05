#include <viua/types/integer.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
#include <viua/scheduler/vps.h>
using namespace std;


byte* Process::optry(byte* addr) {
    /** Create new special frame for try blocks.
     */
    if (try_frame_new) {
        throw "new block frame requested while last one is unused";
    }
    try_frame_new.reset(new TryFrame());
    return addr;
}

byte* Process::opcatch(byte* addr) {
    /** Run catch instruction.
     */
    string type_name = viua::operand::extractString(addr);
    string catcher_block_name = viua::operand::extractString(addr);

    if (not scheduler->isBlock(catcher_block_name)) {
        throw new Exception("registering undefined handler block '" + catcher_block_name + "' to handle " + type_name);
    }

    try_frame_new->catchers[type_name] = new Catcher(type_name, catcher_block_name);

    return addr;
}

byte* Process::oppull(byte* addr) {
    /** Run pull instruction.
     */
    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    if (not caught) {
        throw new Exception("no caught object to pull");
    }
    uregset->set(target, caught.release());

    return addr;
}

byte* Process::openter(byte* addr) {
    /*  Run enter instruction.
     */
    string block_name = viua::operand::extractString(addr);

    if (not scheduler->isBlock(block_name)) {
        throw new Exception("cannot enter undefined block: " + block_name);
    }

    byte* block_address = adjustJumpBaseForBlock(block_name);

    try_frame_new->return_address = addr; // address has already been adjusted by extractString()
    try_frame_new->associated_frame = frames.back();
    try_frame_new->block_name = block_name;

    tryframes.push_back(try_frame_new.release());

    return block_address;
}

byte* Process::opthrow(byte* addr) {
    /** Run throw instruction.
     */
    unsigned source = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    if (source >= uregset->size()) {
        ostringstream oss;
        oss << "invalid read: register out of bounds: " << source;
        throw new Exception(oss.str());
    }
    if (uregset->at(source) == nullptr) {
        ostringstream oss;
        oss << "invalid throw: register " << source << " is empty";
        throw new Exception(oss.str());
    }

    thrown.reset(uregset->pop(source));

    return addr;
}

byte* Process::opleave(byte* addr) {
    /*  Run leave instruction.
     */
    if (tryframes.size() == 0) {
        throw new Exception("bad leave: no block has been entered");
    }
    addr = tryframes.back()->return_address;
    delete tryframes.back();
    tryframes.pop_back();

    if (frames.size() > 0) {
        adjustJumpBaseFor(frames.back()->function_name);
    }
    return addr;
}
