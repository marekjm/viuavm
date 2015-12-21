#include <viua/types/boolean.h>
#include <viua/types/reference.h>
#include <viua/types/thread.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::opthread(byte* addr) {
    /*  Run thread instruction.
     */
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    string call_name = viua::operand::extractString(addr);

    bool is_native = (cpu->function_addresses.count(call_name) or cpu->linked_functions.count(call_name));
    bool is_foreign = cpu->foreign_functions.count(call_name);

    if (not (is_native or is_foreign)) {
        throw new Exception("call to undefined function: " + call_name);
    }

    frame_new->function_name = call_name;
    Thread* vm_thread = cpu->spawn(frame_new);
    ThreadType* thrd = new ThreadType(vm_thread);
    place(target, thrd);
    frame_new = nullptr;

    return addr;
}
byte* Thread::opthjoin(byte* addr) {
    /** Join a thread.
     *
     *  This opcode blocks execution of current thread until
     *  the thread being joined finishes execution.
     */
    byte* return_addr = (addr-1);

    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    if (ThreadType* thrd = dynamic_cast<ThreadType*>(fetch(target))) {
        if (thrd->stopped()) {
            thrd->join();
            return_addr = addr;
            if (thrd->terminated()) {
                thrd->transferActiveExceptionTo(thrown);
            }
        }
    } else {
        throw new Exception("invalid type: expected Thread");
    }

    return return_addr;
}
byte* Thread::opthreceive(byte* addr) {
    /** Receive a message.
     *
     *  This opcode blocks execution of current thread
     *  until a message arrives.
     */
    byte* return_addr = (addr-1);

    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    if (message_queue.size()) {
        place(target, message_queue.front());
        message_queue.pop();
        return_addr = addr;
    }

    return return_addr;
}
