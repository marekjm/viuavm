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
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
using namespace std;


viua::types::Type* viua::process::Process::fetch(viua::internals::types::register_index index) const {
    /*  Return pointer to object at given register.
     *  This method safeguards against reaching for out-of-bounds registers and
     *  reading from an empty register.
     */
    viua::types::Type* object = currently_used_register_set->get(index);
    if (dynamic_cast<viua::types::Reference*>(object)) {
        object = static_cast<viua::types::Reference*>(object)->pointsTo();
    }
    return object;
}
viua::types::Type* viua::process::Process::obtain(viua::internals::types::register_index index) const {
    return fetch(index);
}

viua::kernel::Register* viua::process::Process::register_at(viua::internals::types::register_index i) {
    return currently_used_register_set->register_at(i);
}

unique_ptr<viua::types::Type> viua::process::Process::pop(viua::internals::types::register_index index) {
    return currently_used_register_set->pop(index);
}
void viua::process::Process::place(viua::internals::types::register_index index, unique_ptr<viua::types::Type> obj) {
    /** Place an object in register with given index.
     *
     *  Before placing an object in register, a check is preformed if the register is empty.
     *  If not - the `viua::types::Type` previously stored in it is destroyed.
     *
     */
    currently_used_register_set->set(index, std::move(obj));
}
void viua::process::Process::put(viua::internals::types::register_index index, unique_ptr<viua::types::Type> o) {
    place(index, std::move(o));
}
void viua::process::Process::ensureStaticRegisters(string function_name) {
    /** Makes sure that static register set for requested function is initialized.
     */
    try {
        static_registers.at(function_name);
    } catch (const std::out_of_range& e) {
        // FIXME: amount of static registers should be customizable
        static_registers[function_name] = unique_ptr<viua::kernel::RegisterSet>(new viua::kernel::RegisterSet(16));
    }
}

Frame* viua::process::Process::requestNewFrame(viua::internals::types::register_index arguments_size, viua::internals::types::register_index registers_size) {
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
void viua::process::Process::pushFrame() {
    /** Pushes new frame to be the current (top-most) one.
     */
    if (frames.size() > MAX_STACK_SIZE) {
        ostringstream oss;
        oss << "stack size (" << MAX_STACK_SIZE << ") exceeded with call to '" << frame_new->function_name << '\'';
        throw new viua::types::Exception(oss.str());
    }

    currently_used_register_set = frame_new->local_register_set.get();
    if (find(frames.begin(), frames.end(), frame_new) != frames.end()) {
        ostringstream oss;
        oss << "stack corruption: frame " << hex << frame_new.get() << dec << " for function " << frame_new->function_name << '/' << frame_new->arguments->size() << " pushed more than once";
        throw oss.str();
    }
    frames.emplace_back(std::move(frame_new));
}
void viua::process::Process::dropFrame() {
    /** Drops top-most frame from call stack.
     */
    unique_ptr<Frame> frame(std::move(frames.back()));
    frames.pop_back();

    for (viua::internals::types::register_index i = 0; i < frame->arguments->size(); ++i) {
        if (frame->arguments->at(i) != nullptr and frame->arguments->isflagged(i, MOVED)) {
            throw new viua::types::Exception("unused pass-by-move parameter");
        }
    }

    if (frames.size() == 0) {
        return_value = frame->local_register_set->pop(0);
    }

    if (frames.size()) {
        currently_used_register_set = frames.back()->local_register_set.get();
    } else {
        currently_used_register_set = global_register_set.get();
    }
}
void viua::process::Process::popFrame() {
    // popFrame() is a public function for dropping frames
    dropFrame();
}

viua::internals::types::byte* viua::process::Process::adjustJumpBaseForBlock(const string& call_name) {
    viua::internals::types::byte *entry_point = nullptr;
    auto ep = scheduler->getEntryPointOfBlock(call_name);
    entry_point = ep.first;
    jump_base = ep.second;
    return entry_point;
}
viua::internals::types::byte* viua::process::Process::adjustJumpBaseFor(const string& call_name) {
    viua::internals::types::byte *entry_point = nullptr;
    auto ep = scheduler->getEntryPointOf(call_name);
    entry_point = ep.first;
    jump_base = ep.second;
    return entry_point;
}
viua::internals::types::byte* viua::process::Process::callNative(viua::internals::types::byte* return_address, const string& call_name, viua::kernel::Register* return_register, const string&) {
    viua::internals::types::byte* call_address = adjustJumpBaseFor(call_name);

    if (not frame_new) {
        throw new viua::types::Exception("function call without a frame: use `frame 0' in source code if the function takes no parameters");
    }

    frame_new->function_name = call_name;
    frame_new->return_address = return_address;
    frame_new->return_register = return_register;

    pushFrame();

    return call_address;
}
viua::internals::types::byte* viua::process::Process::callForeign(viua::internals::types::byte* return_address, const string& call_name, viua::kernel::Register* return_register, const string&) {
    if (not frame_new) {
        throw new viua::types::Exception("external function call without a frame: use `frame 0' in source code if the function takes no parameters");
    }

    frame_new->function_name = call_name;
    frame_new->return_address = return_address;
    frame_new->return_register = return_register;

    suspend();
    scheduler->requestForeignFunctionCall(frame_new.release(), this);

    return return_address;
}
viua::internals::types::byte* viua::process::Process::callForeignMethod(viua::internals::types::byte* return_address, viua::types::Type* object, const string& call_name, viua::kernel::Register* return_register, const string&) {
    if (not frame_new) {
        throw new viua::types::Exception("foreign method call without a frame");
    }

    frame_new->function_name = call_name;
    frame_new->return_address = return_address;
    frame_new->return_register = return_register;

    Frame* frame = frame_new.get();

    pushFrame();

    if (not scheduler->isForeignMethod(call_name)) {
        throw new viua::types::Exception("call to unregistered foreign method: " + call_name);
    }

    viua::types::Reference* rf = nullptr;
    if ((rf = dynamic_cast<viua::types::Reference*>(object))) {
        object = rf->pointsTo();
    }

    try {
        // FIXME: supply static and global registers to foreign functions
        scheduler->requestForeignMethodCall(call_name, object, frame, nullptr, nullptr, this);
    } catch (const std::out_of_range& e) {
        throw new viua::types::Exception(e.what());
    }

    // FIXME: woohoo! segfault!
    unique_ptr<viua::types::Type> returned;
    if (return_register != nullptr) {
        // we check in 0. register because it's reserved for return values
        if (currently_used_register_set->at(0) == nullptr) {
            throw new viua::types::Exception("return value requested by frame but foreign method did not set return register");
        }
        returned = currently_used_register_set->pop(0);
    }

    dropFrame();

    // place return value
    if (returned and frames.size() > 0) {
        *return_register = std::move(returned);
    }

    return return_address;
}

void viua::process::Process::adjustInstructionPointer(TryFrame* tframe, string handler_found_for_type) {
    instruction_pointer = adjustJumpBaseForBlock(tframe->catchers.at(handler_found_for_type)->catcher_name);
}
void viua::process::Process::unwindCallStack(TryFrame* tframe) {
    decltype(frames)::size_type distance = 0;
    for (decltype(frames)::size_type j = (frames.size()-1); j > 1; --j) {
        if (frames[j].get() == tframe->associated_frame) {
            break;
        }
        ++distance;
    }
    for (decltype(distance) j = 0; j < distance; ++j) {
        dropFrame();
    }
}
void viua::process::Process::unwindTryStack(TryFrame* tframe) {
    while (tryframes.back().get() != tframe) {
        tryframes.pop_back();
    }
}
void viua::process::Process::unwindStack(TryFrame* tframe, string handler_found_for_type) {
    adjustInstructionPointer(tframe, handler_found_for_type);
    unwindCallStack(tframe);
    unwindTryStack(tframe);
}
tuple<TryFrame*, string> viua::process::Process::findCatchFrame() {
    TryFrame* found_exception_frame = nullptr;
    string caught_with_type = "";

    for (decltype(tryframes)::size_type i = tryframes.size(); i > 0; --i) {
        TryFrame* tframe = tryframes[(i-1)].get();
        string handler_found_for_type = thrown->type();
        bool handler_found = tframe->catchers.count(handler_found_for_type);

        // FIXME: mutex
        if ((not handler_found) and scheduler->isClass(handler_found_for_type)) {
            vector<string> types_to_check = scheduler->inheritanceChainOf(handler_found_for_type);
            for (decltype(types_to_check)::size_type j = 0; j < types_to_check.size(); ++j) {
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
void viua::process::Process::handleActiveException() {
    TryFrame* tframe = nullptr;
    string handler_found_for_type = "";

    tie(tframe, handler_found_for_type) = findCatchFrame();
    if (tframe != nullptr) {
        unwindStack(tframe, handler_found_for_type);
        caught.reset(thrown.release());
    }
}
viua::internals::types::byte* viua::process::Process::tick() {
    bool halt = false;

    viua::internals::types::byte* previous_instruction_pointer = instruction_pointer;
    ++instruction_counter;

    try {
        instruction_pointer = dispatch(instruction_pointer);
    } catch (viua::types::Exception* e) {
        /* All machine-thrown exceptions are passed back to user code.
         * This is much easier than checking for erroneous conditions and
         * terminating functions conditionally, instead - machine just throws viua::types::Exception objects which
         * are then caught here.
         *
         * If user code cannot deal with them (i.e. did not register a catcher block) they will terminate execution later.
         */
        thrown.reset(e);
    } catch (const HaltException& e) {
        halt = true;
    } catch (viua::types::Type* e) {
        thrown.reset(e);
    } catch (const char* e) {
        thrown.reset(new viua::types::Exception(e));
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
        thrown.reset(new viua::types::Exception("InstructionUnchanged"));
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

void viua::process::Process::join() {
    /** Join a process with calling process.
     *
     *  This function causes calling process to be blocked until
     *  this process has stopped.
     */
    is_joinable.store(false, std::memory_order_release);
}
void viua::process::Process::detach() {
    /** Detach a process.
     *
     *  This function causes the process to become unjoinable, but
     *  allows it to run in the background.
     *
     *  Keep in mind that while detached processes cannot be joined,
     *  they can receive messages.
     *  Also, they will run even after the main/ function has exited.
     */
    is_joinable.store(false, std::memory_order_release);
    parent_process = nullptr;
}
bool viua::process::Process::joinable() const {
    return is_joinable.load(std::memory_order_acquire);
}

void viua::process::Process::suspend() {
    is_suspended.store(true, std::memory_order_release);
}
void viua::process::Process::wakeup() {
    is_suspended.store(false, std::memory_order_release);
}
bool viua::process::Process::suspended() const {
    return is_suspended.load(std::memory_order_acquire);
}

viua::process::Process* viua::process::Process::parent() const {
    return parent_process;
}

string viua::process::Process::starting_function() const {
    return entry_function;
}

auto viua::process::Process::priority() const -> decltype(process_priority) {
    return process_priority;
}
void viua::process::Process::priority(decltype(process_priority) p) {
    process_priority = p;
}

bool viua::process::Process::stopped() const {
    return (finished or terminated());
}

bool viua::process::Process::terminated() const {
    return static_cast<bool>(thrown);
}

void viua::process::Process::pass(unique_ptr<viua::types::Type> message) {
    message_queue.push(std::move(message));
    wakeup();
}


viua::types::Type* viua::process::Process::getActiveException() {
    return thrown.get();
}

unique_ptr<viua::types::Type> viua::process::Process::transferActiveException() {
    return std::move(thrown);
}

void viua::process::Process::raise(unique_ptr<viua::types::Type> exception) {
    thrown = std::move(exception);
}


unique_ptr<viua::types::Type> viua::process::Process::getReturnValue() {
    return std::move(return_value);
}

bool viua::process::Process::watchdogged() const {
    return (not watchdog_function.empty());
}
string viua::process::Process::watchdog() const {
    return watchdog_function;
}
viua::internals::types::byte* viua::process::Process::become(const string& function_name, std::unique_ptr<Frame> frame_to_use) {
    if (not scheduler->isNativeFunction(function_name)) {
        throw new viua::types::Exception("process from undefined function: " + function_name);
    }

    frames.clear();
    thrown.reset(nullptr);
    caught.reset(nullptr);
    finished = false;

    frame_to_use->function_name = function_name;
    frame_new = std::move(frame_to_use);

    pushFrame();

    return (instruction_pointer = adjustJumpBaseFor(function_name));
}

viua::internals::types::byte* viua::process::Process::begin() {
    if (not scheduler->isNativeFunction(frames[0]->function_name)) {
        throw new viua::types::Exception("process from undefined function: " + frames[0]->function_name);
    }
    return (instruction_pointer = adjustJumpBaseFor(frames[0]->function_name));
}
auto viua::process::Process::counter() const -> decltype(instruction_counter) {
    return instruction_counter;
}
auto viua::process::Process::executionAt() const -> decltype(instruction_pointer) {
    return instruction_pointer;
}


vector<Frame*> viua::process::Process::trace() const {
    vector<Frame*> tr;
    for (auto& each : frames) {
        tr.push_back(each.get());
    }
    return tr;
}

viua::process::PID viua::process::Process::pid() const {
    return process_id;
}
bool viua::process::Process::hidden() const {
    return is_hidden;
}
void viua::process::Process::hidden(bool state) {
    is_hidden = state;
}

bool viua::process::Process::empty() const {
    return message_queue.empty();
}

void viua::process::Process::migrate_to(viua::scheduler::VirtualProcessScheduler *sch) {
    scheduler = sch;
}

viua::process::Process::Process(unique_ptr<Frame> frm, viua::scheduler::VirtualProcessScheduler *sch, viua::process::Process* pt): scheduler(sch), parent_process(pt), entry_function(frm->function_name),
    global_register_set(nullptr), currently_used_register_set(nullptr), tmp(nullptr),
    jump_base(nullptr),
    frame_new(nullptr), try_frame_new(nullptr),
    thrown(nullptr), caught(nullptr),
    return_value(nullptr),
    instruction_counter(0),
    instruction_pointer(nullptr),
    finished(false), is_joinable(true),
    is_suspended(false),
    process_priority(1),
    process_id(this),
    is_hidden(false)
{
    global_register_set.reset(new viua::kernel::RegisterSet(DEFAULT_REGISTER_SIZE));
    currently_used_register_set = frm->local_register_set.get();
    frames.emplace_back(std::move(frm));
}

viua::process::Process::~Process() {}
