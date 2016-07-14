/*
 *  Copyright (C) 2015, 2016 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

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
#include <viua/scheduler/vps.h>
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
Type* Process::obtain(unsigned index) const {
    return fetch(index);
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
void Process::put(unsigned index, Type *o) {
    place(index, o);
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
        static_registers[function_name] = unique_ptr<RegisterSet>(new RegisterSet(16));
    }
}

Frame* Process::requestNewFrame(unsigned arguments_size, unsigned registers_size) {
    /** Request new frame to be prepared.
     *
     *  Creates new frame if the new-frame hook is empty.
     *  Throws an exception otherwise.
     *  Returns pointer to the newly created frame.
     */
    if (frame_new) { throw "requested new frame while last one is unused"; }
    frame_new.reset(new Frame(nullptr, arguments_size, registers_size));
    return frame_new.get();
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
        oss << "stack corruption: frame " << hex << frame_new.get() << dec << " for function " << frame_new->function_name << '/' << frame_new->args->size() << " pushed more than once";
        throw oss.str();
    }
    frames.push_back(std::move(frame_new));
}
void Process::dropFrame() {
    /** Drops top-most frame from call stack.
     */
    unique_ptr<Frame> frame(std::move(frames.back()));
    frames.pop_back();

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

    if (frames.size() == 0) {
        return_value.reset(frame->regset->pop(0));
    }

    if (frames.size()) {
        uregset = frames.back()->regset;
    } else {
        uregset = regset.get();
    }
}
void Process::popFrame() {
    // popFrame() is a public function for dropping frames
    dropFrame();
}

byte* Process::adjustJumpBaseForBlock(const string& call_name) {
    byte *entry_point = nullptr;
    auto ep = scheduler->getEntryPointOfBlock(call_name);
    entry_point = ep.first;
    jump_base = ep.second;
    return entry_point;
}
byte* Process::adjustJumpBaseFor(const string& call_name) {
    byte *entry_point = nullptr;
    auto ep = scheduler->getEntryPointOf(call_name);
    entry_point = ep.first;
    jump_base = ep.second;
    return entry_point;
}
byte* Process::callNative(byte* return_address, const string& call_name, const bool return_ref, const unsigned return_index, const string&) {
    byte* call_address = adjustJumpBaseFor(call_name);

    if (not frame_new) {
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
byte* Process::callForeign(byte* return_address, const string& call_name, const bool return_ref, const unsigned return_index, const string&) {
    if (not frame_new) {
        throw new Exception("external function call without a frame: use `frame 0' in source code if the function takes no parameters");
    }
    // set function name and return address
    frame_new->function_name = call_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = return_ref;
    frame_new->place_return_value_in = return_index;

    suspend();
    scheduler->requestForeignFunctionCall(frame_new.release(), this);

    return return_address;
}
byte* Process::callForeignMethod(byte* return_address, Type* object, const string& call_name, const bool return_ref, const unsigned return_index, const string&) {
    if (not frame_new) {
        throw new Exception("foreign method call without a frame");
    }
    // set function name and return address
    frame_new->function_name = call_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = return_ref;
    frame_new->place_return_value_in = return_index;

    Frame* frame = frame_new.get();

    pushFrame();

    if (not scheduler->isForeignMethod(call_name)) {
        throw new Exception("call to unregistered foreign method: " + call_name);
    }

    Reference* rf = nullptr;
    if ((rf = dynamic_cast<Reference*>(object))) {
        object = rf->pointsTo();
    }

    try {
        // FIXME: supply static and global registers to foreign functions
        scheduler->requestForeignMethodCall(call_name, object, frame, nullptr, nullptr, this);
    } catch (const std::out_of_range& e) {
        throw new Exception(e.what());
    }

    // FIXME: woohoo! segfault!
    Type* returned = nullptr;
    bool returned_is_reference = false;
    unsigned return_value_register = frames.back()->place_return_value_in;
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
            return_value_register = static_cast<Integer*>(fetch(return_value_register))->as_unsigned();
        }
        place(return_value_register, returned);
        if (returned_is_reference) {
            uregset->flag(return_value_register, REFERENCE);
        }
    }

    return return_address;
}

void Process::adjustInstructionPointer(TryFrame* tframe, string handler_found_for_type) {
    instruction_pointer = adjustJumpBaseForBlock(tframe->catchers.at(handler_found_for_type)->catcher_name);
}
void Process::unwindCallStack(TryFrame* tframe) {
    unsigned distance = 0;
    for (long unsigned j = (frames.size()-1); j > 1; --j) {
        if (frames[j].get() == tframe->associated_frame) {
            break;
        }
        ++distance;
    }
    for (unsigned j = 0; j < distance; ++j) {
        dropFrame();
    }
}
void Process::unwindTryStack(TryFrame* tframe) {
    while (tryframes.back().get() != tframe) {
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
        TryFrame* tframe = tryframes[(i-1)].get();
        string handler_found_for_type = thrown->type();
        bool handler_found = tframe->catchers.count(handler_found_for_type);

        // FIXME: mutex
        if ((not handler_found) and scheduler->isClass(handler_found_for_type)) {
            vector<string> types_to_check = scheduler->inheritanceChainOf(handler_found_for_type);
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
        caught.reset(thrown.release());
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
        thrown.reset(e);
    } catch (const HaltException& e) {
        halt = true;
    } catch (Type* e) {
        thrown.reset(e);
    } catch (const char* e) {
        thrown.reset(new Exception(e));
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
    if (instruction_pointer == previous_instruction_pointer and (OPCODE(*instruction_pointer) != RETURN and OPCODE(*instruction_pointer) != JOIN and OPCODE(*instruction_pointer) != RECEIVE) and (not thrown)) {
        thrown.reset(new Exception("InstructionUnchanged"));
    }

    if (thrown and frame_new) {
        /*  Delete active frame after an exception is thrown.
         *  There're two reasons for such behaviour:
         *  - it prevents memory leaks if an exception escapes and
         *    is handled by the watchdog process,
         *  - if the exception is caught, it provides the servicing block with a
         *    clean environment (as there is no way of dropping a frame without
         *    using it),
         */
        frame_new.reset(nullptr);
    }

    if (thrown) {
        handleActiveException();
    }
    if (thrown) {
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

Process* Process::parent() const {
    return parent_process;
}

auto Process::priority() const -> decltype(process_priority) {
    return process_priority;
}
void Process::priority(decltype(process_priority) p) {
    process_priority = p;
}

bool Process::stopped() const {
    return (finished or terminated());
}

bool Process::terminated() const {
    return static_cast<bool>(thrown);
}

void Process::pass(unique_ptr<Type> message) {
    message_queue.push(std::move(message));
    wakeup();
}


Type* Process::getActiveException() {
    return thrown.get();
}

Type* Process::transferActiveException() {
    return thrown.release();
}

void Process::raiseException(Type *exception) {
    thrown.reset(exception);
}


unique_ptr<Type> Process::getReturnValue() {
    return std::move(return_value);
}


byte* Process::begin() {
    if (not scheduler->isNativeFunction(frames[0]->function_name)) {
        throw new Exception("process from undefined function: " + frames[0]->function_name);
    }
    return (instruction_pointer = adjustJumpBaseFor(frames[0]->function_name));
}
uint64_t Process::counter() const {
    return instruction_counter;
}
auto Process::executionAt() const -> decltype(instruction_pointer) {
    return instruction_pointer;
}


vector<Frame*> Process::trace() const {
    vector<Frame*> tr;
    for (auto& each : frames) {
        tr.push_back(each.get());
    }
    return tr;
}


Process::Process(unique_ptr<Frame> frm, viua::scheduler::VirtualProcessScheduler *sch, Process* pt): scheduler(sch), parent_process(pt), entry_function(frm->function_name),
    regset(nullptr), uregset(nullptr), tmp(nullptr),
    jump_base(nullptr),
    frame_new(nullptr), try_frame_new(nullptr),
    thrown(nullptr), caught(nullptr),
    return_value(nullptr),
    instruction_counter(0),
    instruction_pointer(nullptr),
    finished(false), is_joinable(true),
    is_suspended(false),
    process_priority(1)
{
    regset.reset(new RegisterSet(DEFAULT_REGISTER_SIZE));
    uregset = frm->regset;
    frames.push_back(std::move(frm));
}

Process::~Process() {}
