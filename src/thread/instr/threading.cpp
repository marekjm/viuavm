#include <viua/types/boolean.h>
#include <viua/types/reference.h>
#include <viua/types/thread.h>
#include <viua/support/pointer.h>
#include <viua/exceptions.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::opthread(byte* addr) {
    /*  Run thread instruction.
     */
    bool return_register_ref = *(bool*)addr;
    pointer::inc<bool, byte>(addr);

    int return_register_index = *(int*)addr;
    pointer::inc<int, byte>(addr);

    string call_name = string(addr);

    bool is_native = (cpu->function_addresses.count(call_name) or cpu->linked_functions.count(call_name));
    bool is_foreign = cpu->foreign_functions.count(call_name);

    if (not (is_native or is_foreign)) {
        throw new Exception("call to undefined function: " + call_name);
    }

    if (return_register_ref) {
        return_register_index = static_cast<Integer*>(fetch(return_register_index))->value();
    }

    frame_new->function_name = call_name;
    Thread* vm_thread = cpu->spawn(frame_new);
    ThreadType* thrd = new ThreadType(vm_thread);
    place(return_register_index, thrd);
    frame_new = nullptr;

    return (addr+call_name.size()+1);
}
byte* Thread::opthjoin(byte* addr) {
    /** Join a thread.
     *
     *  This opcode blocks execution of current thread until
     */
    byte* return_addr = (addr-1);

    bool thrd_register_ref = *(bool*)addr;
    pointer::inc<bool, byte>(addr);

    int thrd_register_index = *(int*)addr;
    pointer::inc<int, byte>(addr);

    if (thrd_register_ref) {
        thrd_register_index = static_cast<Integer*>(fetch(thrd_register_index))->value();
    }

    if (ThreadType* thrd = dynamic_cast<ThreadType*>(fetch(thrd_register_index))) {
        if (thrd->stopped()) {
            thrd->join();
            return_addr = addr;
        }
    } else {
        throw new Exception("invalid type: expected Thread");
    }

    return return_addr;
}
byte* Thread::opthdetach(byte* addr) {
    /** Detach a thread.
     */
    bool thrd_register_ref = *(bool*)addr;
    pointer::inc<bool, byte>(addr);

    int thrd_register_index = *(int*)addr;
    pointer::inc<int, byte>(addr);

    if (thrd_register_ref) {
        thrd_register_index = static_cast<Integer*>(fetch(thrd_register_index))->value();
    }

    if (ThreadType* thrd = dynamic_cast<ThreadType*>(fetch(thrd_register_index))) {
        thrd->detach();
    } else {
        throw new Exception("invalid type: expected Thread");
    }

    return addr;
}
byte* Thread::opthreceive(byte* addr) {
    /** Receive a message.
     *
     *  This opcode blocks execution of current thread
     *  until a message arrives.
     */
    byte* return_addr = (addr-1);

    bool return_register_ref = *(bool*)addr;
    pointer::inc<bool, byte>(addr);

    int return_register_index = *(int*)addr;
    pointer::inc<int, byte>(addr);

    if (return_register_ref) {
        return_register_index = static_cast<Integer*>(fetch(return_register_index))->value();
    }

    if (message_queue.size()) {
        place(return_register_index, message_queue.front());
        message_queue.pop();
        return_addr = addr;
    }

    return return_addr;
}
