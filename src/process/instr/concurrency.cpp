#include <viua/types/boolean.h>
#include <viua/types/reference.h>
#include <viua/types/process.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
#include <viua/scheduler/vps.h>
using namespace std;


byte* Process::opprocess(byte* addr) {
    /*  Run process instruction.
     */
    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    string call_name = viua::operand::extractString(addr);

    bool is_native = scheduler->isNativeFunction(call_name);
    bool is_foreign = scheduler->isForeignFunction(call_name);

    if (not (is_native or is_foreign)) {
        throw new Exception("call to undefined function: " + call_name);
    }

    frame_new->function_name = call_name;
    place(target, new ProcessType(scheduler->spawn(frame_new, this)));
    frame_new = nullptr;

    return addr;
}
byte* Process::opjoin(byte* addr) {
    /** Join a process.
     *
     *  This opcode blocks execution of current process until
     *  the process being joined finishes execution.
     */
    byte* return_addr = (addr-1);

    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    unsigned source = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    if (ProcessType* thrd = dynamic_cast<ProcessType*>(fetch(source))) {
        if (thrd->stopped()) {
            thrd->join();
            return_addr = addr;
            if (thrd->terminated()) {
                thrown = thrd->transferActiveException();
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
byte* Process::opreceive(byte* addr) {
    /** Receive a message.
     *
     *  This opcode blocks execution of current process
     *  until a message arrives.
     */
    byte* return_addr = (addr-1);

    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

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

    bool is_native = scheduler->isNativeFunction(call_name);
    bool is_foreign = scheduler->isForeignFunction(call_name);

    if (not (is_native or is_foreign)) {
        throw new Exception("watchdog process from undefined function: " + call_name);
    }
    if (not is_native) {
        throw new Exception("watchdog process must be a native function, used foreign " + call_name);
    }

    frame_new->function_name = call_name;
    scheduler->cpu()->spawnWatchdog(frame_new);
    frame_new = nullptr;

    return addr;
}
