#include <viua/types/boolean.h>
#include <viua/types/reference.h>
#include <viua/types/process.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Process::opthread(byte* addr) {
    /*  Run thread instruction.
     */
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    string call_name = viua::operand::extractString(addr);

    bool is_native = (cpu->function_addresses.count(call_name) or cpu->linked_functions.count(call_name));
    bool is_foreign = cpu->foreign_functions.count(call_name);

    if (not (is_native or is_foreign)) {
        throw new Exception("call to undefined function: " + call_name);
    }

    // clear PASSED flag
    // since we copy all values when creating processes
    // we can safely overwrite all registers after the process has launched
    for (unsigned i = 0; i < uregset->size(); ++i) {
        if (uregset->at(i) != nullptr) {
            uregset->unflag(i, PASSED);
        }
    }

    frame_new->captureArguments();

    frame_new->function_name = call_name;
    place(target, new ProcessType(cpu->spawn(frame_new, this)));
    frame_new = nullptr;

    return addr;
}
byte* Process::opthjoin(byte* addr) {
    /** Join a process.
     *
     *  This opcode blocks execution of current process until
     *  the process being joined finishes execution.
     */
    byte* return_addr = (addr-1);

    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    int source = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    if (ProcessType* thrd = dynamic_cast<ProcessType*>(fetch(source))) {
        if (thrd->stopped()) {
            thrd->join();
            return_addr = addr;
            if (thrd->terminated()) {
                thrd->transferActiveExceptionTo(thrown);
            }
            if (target) {
                place(target, thrd->getReturnValue());
            }
        }
    } else {
        throw new Exception("invalid type: expected Process");
    }

    return return_addr;
}
byte* Process::opthreceive(byte* addr) {
    /** Receive a message.
     *
     *  This opcode blocks execution of current process
     *  until a message arrives.
     */
    byte* return_addr = (addr-1);

    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    if (message_queue.size()) {
        place(target, message_queue.front());
        message_queue.pop();
        return_addr = addr;
    } else {
        suspend();
    }

    return return_addr;
}
byte* Process::opwatchdog(byte* addr) {
    /*  Run watchdog instruction.
     */
    string call_name = viua::operand::extractString(addr);

    bool is_native = (cpu->function_addresses.count(call_name) or cpu->linked_functions.count(call_name));
    bool is_foreign = cpu->foreign_functions.count(call_name);

    if (not (is_native or is_foreign)) {
        throw new Exception("watchdog process from undefined function: " + call_name);
    }
    if (not is_native) {
        throw new Exception("watchdog process must be native function, used foreign " + call_name);
    }

    // clear PASSED flag
    // since we copy all values when creating processes
    // we can safely overwrite all registers after the process has launched
    for (unsigned i = 0; i < uregset->size(); ++i) {
        if (uregset->at(i) != nullptr) {
            uregset->unflag(i, PASSED);
        }
    }

    frame_new->captureArguments();

    frame_new->function_name = call_name;
    cpu->spawnWatchdog(frame_new);
    frame_new = nullptr;

    return addr;
}
