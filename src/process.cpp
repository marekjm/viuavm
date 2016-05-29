#include <iostream>
#include <algorithm>
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/maps.h>
#include <viua/types/integer.h>
#include <viua/types/exception.h>
#include <viua/types/reference.h>
#include <viua/types/process.h>
#include <viua/process.h>
#include <viua/cpu/cpu.h>
using namespace std;


Type* Process::fetch(unsigned index) const {
    /*  Return pointer to object at given register.
     *  This method safeguards against reaching for out-of-bounds registers and
     *  reading from an empty register.
     *
     *  :params:
     *
     *  index:int   - index of a register to fetch
     */
    Type* object = uregset->get(index);
    if (dynamic_cast<Reference*>(object)) {
        object = static_cast<Reference*>(object)->pointsTo();
    }
    return object;
}
Type* Process::pop(unsigned index) {
    /*  Return pointer to object at given register.
     *  The object is removed from the register.
     *  This method safeguards against reaching for out-of-bounds registers and
     *  reading from an empty register.
     *
     *  :params:
     *
     *  index:int   - index of a register to pop an object from
     */
    return uregset->pop(index);
}
void Process::place(unsigned index, Type* obj) {
    /** Place an object in register with given index.
     *
     *  Before placing an object in register, a check is preformed if the register is empty.
     *  If not - the `Type` previously stored in it is destroyed.
     *
     */
    Type* old_ref_ptr = (hasrefs(index) ? uregset->at(index) : nullptr);
    uregset->set(index, obj);

    // update references *if, and only if* the register being set has references and
    // is *not marked a reference* itself, i.e. is the origin register
    if (old_ref_ptr and not uregset->isflagged(index, REFERENCE)) {
        updaterefs(old_ref_ptr, obj);
    }
}
void Process::updaterefs(Type* before, Type* now) {
    /** This method updates references to a given address present in registers.
     *  It swaps old address for the new one in every register that points to the old address.
     *
     *  There is no need to delete old object in this function, as it will be deleted as soon as
     *  it is replaced in the origin register (i.e. the register that holds the original pointer to
     *  the object - the one from which all references had been derived).
     */
    // FIXME: this function should update references in all registersets
    for (unsigned i = 0; i < uregset->size(); ++i) {
        if (uregset->at(i) == before) {
            if (debug) {
                cout << "\nCPU: updating reference address in register " << i << hex << ": " << before << " -> " << now << dec << endl;
            }
            mask_t had_mask = uregset->getmask(i);
            uregset->empty(i);
            uregset->set(i, now);
            uregset->setmask(i, had_mask);
        }
    }
}
bool Process::hasrefs(unsigned index) {
    /** This method checks if object at a given address exists as a reference in another register.
     */
    bool has = false;
    // FIXME: this should check for references in every register set; gonna be slow, isn't it?
    for (unsigned i = 0; i < uregset->size(); ++i) {
        if (i == index) continue;
        if (uregset->at(i) == uregset->at(index)) {
            has = true;
            break;
        }
    }
    return has;
}
void Process::ensureStaticRegisters(string function_name) {
    /** Makes sure that static register set for requested function is initialized.
     */
    try {
        static_registers.at(function_name);
    } catch (const std::out_of_range& e) {
        // FIXME: amount of static registers should be customizable
        static_registers[function_name] = new RegisterSet(16);
    }
}

Frame* Process::requestNewFrame(int arguments_size, int registers_size) {
    /** Request new frame to be prepared.
     *
     *  Creates new frame if the new-frame hook is empty.
     *  Throws an exception otherwise.
     *  Returns pointer to the newly created frame.
     */
    if (frame_new != nullptr) { throw "requested new frame while last one is unused"; }
    return (frame_new = new Frame(nullptr, arguments_size, registers_size));
}
void Process::pushFrame() {
    /** Pushes new frame to be the current (top-most) one.
     */
    if (frames.size() > MAX_STACK_SIZE) {
        ostringstream oss;
        oss << "stack size (" << MAX_STACK_SIZE << ") exceeded with call to '" << frame_new->function_name << '\'';
        throw new Exception(oss.str());
    }

    uregset = frame_new->regset;
    if (find(frames.begin(), frames.end(), frame_new) != frames.end()) {
        ostringstream oss;
        oss << "stack corruption: frame " << hex << frame_new << dec << " for function " << frame_new->function_name << '/' << frame_new->args->size() << " pushed more than once";
        throw oss.str();
    }
    frames.push_back(frame_new);
    frame_new = nullptr;
}
void Process::dropFrame() {
    /** Drops top-most frame from call stack.
     */
    Frame* frame = frames.back();

    for (registerset_size_type i = 0; i < frame->regset->size(); ++i) {
        if (ProcessType* t = dynamic_cast<ProcessType*>(frame->regset->at(i))) {
            if (t->joinable()) {
                throw new Exception("joinable process in dropped frame");
            }
        }
    }
    for (registerset_size_type i = 0; i < frame->args->size(); ++i) {
        if (frame->args->at(i) != nullptr and frame->args->isflagged(i, MOVED)) {
            throw new Exception("unused pass-by-move parameter");
        }
    }

    if (frames.size() == 1) {
        return_value = frame->regset->pop(0);
    }

    delete frame;
    frames.pop_back();

    if (frames.size()) {
        uregset = frames.back()->regset;
    } else {
        uregset = regset;
    }
}

byte* Process::adjustJumpBaseFor(const string& call_name) {
    byte* call_address = nullptr;
    if (cpu->function_addresses.count(call_name)) {
        call_address = cpu->bytecode+(cpu->function_addresses.at(call_name));
        jump_base = cpu->bytecode;
    } else {
        call_address = cpu->linked_functions.at(call_name).second;
        jump_base = cpu->linked_modules.at(cpu->linked_functions.at(call_name).first).second;
    }
    return call_address;
}
byte* Process::callNative(byte* return_address, const string& call_name, const bool& return_ref, const int& return_index, const string&) {
    byte* call_address = adjustJumpBaseFor(call_name);

    if (frame_new == nullptr) {
        throw new Exception("function call without a frame: use `frame 0' in source code if the function takes no parameters");
    }
    // set function name and return address
    frame_new->function_name = call_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = return_ref;
    frame_new->place_return_value_in = return_index;

    pushFrame();

    return call_address;
}
byte* Process::callForeign(byte* return_address, const string& call_name, const bool& return_ref, const int& return_index, const string&) {
    if (frame_new == nullptr) {
        throw new Exception("external function call without a frame: use `frame 0' in source code if the function takes no parameters");
    }
    // set function name and return address
    frame_new->function_name = call_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = return_ref;
    frame_new->place_return_value_in = return_index;

    suspend();
    cpu->requestForeignFunctionCall(frame_new, this);
    frame_new = nullptr;

    return return_address;
}
byte* Process::callForeignMethod(byte* return_address, Type* object, const string& call_name, const bool& return_ref, const int& return_index, const string&) {
    if (frame_new == nullptr) {
        throw new Exception("foreign method call without a frame");
    }
    // set function name and return address
    frame_new->function_name = call_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = return_ref;
    frame_new->place_return_value_in = return_index;

    Frame* frame = frame_new;

    pushFrame();

    if (cpu->foreign_methods.count(call_name) == 0) {
        throw new Exception("call to unregistered foreign method: " + call_name);
    }

    Reference* rf = nullptr;
    if ((rf = dynamic_cast<Reference*>(object))) {
        object = rf->pointsTo();
    }

    try {
        // FIXME: supply static and global registers to foreign functions
        cpu->foreign_methods.at(call_name)(object, frame, nullptr, nullptr, this, cpu);
    } catch (const std::out_of_range& e) {
        throw new Exception(e.what());
    }

    // FIXME: woohoo! segfault!
    Type* returned = nullptr;
    bool returned_is_reference = false;
    int return_value_register = frames.back()->place_return_value_in;
    bool resolve_return_value_register = frames.back()->resolve_return_value_register;
    if (return_value_register != 0) {
        // we check in 0. register because it's reserved for return values
        if (uregset->at(0) == nullptr) {
            throw new Exception("return value requested by frame but foreign method did not set return register");
        }
        returned = uregset->pop(0);
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

void Process::adjustInstructionPointer(TryFrame* tframe, string handler_found_for_type) {
    instruction_pointer = tframe->catchers.at(handler_found_for_type)->block_address;
}
void Process::unwindCallStack(TryFrame* tframe) {
    unsigned distance = 0;
    for (long unsigned j = (frames.size()-1); j > 1; --j) {
        if (frames[j] == tframe->associated_frame) {
            break;
        }
        ++distance;
    }
    for (unsigned j = 0; j < distance; ++j) {
        dropFrame();
    }
}
void Process::unwindTryStack(TryFrame* tframe) {
    while (tryframes.back() != tframe) {
        delete tryframes.back();
        tryframes.pop_back();
    }
}
void Process::unwindStack(TryFrame* tframe, string handler_found_for_type) {
    adjustInstructionPointer(tframe, handler_found_for_type);
    unwindCallStack(tframe);
    unwindTryStack(tframe);
}
tuple<TryFrame*, string> Process::findCatchFrame() {
    TryFrame* found_exception_frame = nullptr;
    string caught_with_type = "";

    for (long unsigned i = tryframes.size(); i > 0; --i) {
        TryFrame* tframe = tryframes[(i-1)];
        string handler_found_for_type = thrown->type();
        bool handler_found = tframe->catchers.count(handler_found_for_type);

        // FIXME: mutex
        if ((not handler_found) and cpu->typesystem.count(handler_found_for_type)) {
            vector<string> types_to_check = cpu->inheritanceChainOf(handler_found_for_type);
            for (unsigned j = 0; j < types_to_check.size(); ++j) {
                if (tframe->catchers.count(types_to_check[j])) {
                    handler_found = true;
                    handler_found_for_type = types_to_check[j];
                    break;
                }
            }
        }

        if (handler_found) {
            found_exception_frame = tframe;
            caught_with_type = handler_found_for_type;
            break;
        }
    }

    return tuple<TryFrame*, string>(found_exception_frame, caught_with_type);
}
void Process::handleActiveException() {
    TryFrame* tframe = nullptr;
    string handler_found_for_type = "";

    tie(tframe, handler_found_for_type) = findCatchFrame();
    if (tframe != nullptr) {
        unwindStack(tframe, handler_found_for_type);
        caught = thrown;
        thrown = nullptr;
    }
}
byte* Process::tick() {
    bool halt = false;

    byte* previous_instruction_pointer = instruction_pointer;
    ++instruction_counter;

    try {
        instruction_pointer = dispatch(instruction_pointer);
    } catch (Exception* e) {
        /* All machine-thrown exceptions are passed back to user code.
         * This is much easier than checking for erroneous conditions and
         * terminating functions conditionally, instead - machine just throws Exception objects which
         * are then caught here.
         *
         * If user code cannot deal with them (i.e. did not register a catcher block) they will terminate execution later.
         */
        thrown = e;
    } catch (const HaltException& e) {
        halt = true;
    } catch (Type* e) {
        thrown = e;
    } catch (const char* e) {
        thrown = new Exception(e);
    }

    if (halt or frames.size() == 0) {
        finished = true;
        return nullptr;
    }

    /*  Machine should halt execution if previous instruction pointer is the same as current one as
     *  it means that the execution flow is corrupted and
     *  entered an infinite loop.
     *
     *  However, execution *should not* be halted if:
     *      - the offending opcode is RETURN (as this may indicate exiting recursive function),
     *      - the offending opcode is JOIN (as this means that a process is waiting for another process to finish),
     *      - the offending opcode is RECEIVE (as this means that a process is waiting for a message),
     *      - an object has been thrown, as the instruction pointer will be adjusted by
     *        catchers or execution will be halted on unhandled types,
     */
    if (instruction_pointer == previous_instruction_pointer and (OPCODE(*instruction_pointer) != RETURN and OPCODE(*instruction_pointer) != JOIN and OPCODE(*instruction_pointer) != RECEIVE) and thrown == nullptr) {
        thrown = new Exception("InstructionUnchanged");
    }

    if (thrown != nullptr and frame_new != nullptr) {
        /*  Delete active frame after an exception is thrown.
         *  There're two reasons for such behaviour:
         *  - it prevents memory leaks if an exception escapes and
         *    is handled by the watchdog process,
         *  - if the exception is caught, it provides the servicing block with a
         *    clean environment (as there is no way of dropping a frame without
         *    using it),
         */
        delete frame_new;
        frame_new = nullptr;
    }

    if (thrown != nullptr) {
        handleActiveException();
    }
    if (thrown != nullptr) {
        has_unhandled_exception = true;
        return nullptr;
    }

    return instruction_pointer;
}

void Process::join() {
    /** Join a process with calling process.
     *
     *  This function causes calling process to be blocked until
     *  this process has stopped.
     */
    is_joinable.store(false, std::memory_order_release);
}
void Process::detach() {
    /** Detach a process.
     *
     *  This function causes the process to become unjoinable, but
     *  allows it to run in the background.
     *
     *  Keep in mind that while detached processes cannot be joined,
     *  they can receive messages.
     *  Also, they will run even after the main/1 function has exited.
     */
    is_joinable.store(false, std::memory_order_release);
    parent_process = nullptr;
}
bool Process::joinable() const {
    return is_joinable.load(std::memory_order_acquire);
}

void Process::suspend() {
    is_suspended.store(true, std::memory_order_release);
}
void Process::wakeup() {
    is_suspended.store(false, std::memory_order_release);
}
bool Process::suspended() {
    return is_suspended.load(std::memory_order_acquire);
}

void Process::pass(Type* message) {
    message_queue.push(message);
    wakeup();
}

byte* Process::begin() {
    if (cpu->function_addresses.count(frames[0]->function_name) == 0) {
        throw new Exception("process from undefined function: " + frames[0]->function_name);
    }
    return (instruction_pointer = (cpu->bytecode + cpu->function_addresses.at(frames[0]->function_name)));
}

Process::~Process() {
    while (frames.size()) {
        delete frames.back();
        frames.pop_back();
    }

    decltype(static_registers)::iterator sr = static_registers.begin();
    while (sr != static_registers.end()) {
        auto rkey = sr->first;
        auto rset = sr->second;
        ++sr;
        static_registers.erase(rkey);
        delete rset;
    }

    if (return_value != nullptr) {
        delete return_value;
    }
}
