/*
 *  Copyright (C) 2015, 2016, 2017, 2018 Marek Marecki
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

#include <algorithm>
#include <iostream>
#include <memory>
#include <viua/bytecode/maps.h>
#include <viua/bytecode/opcodes.h>
#include <viua/kernel/kernel.h>
#include <viua/process.h>
#include <viua/scheduler/vps.h>
#include <viua/types/exception.h>
#include <viua/types/integer.h>
#include <viua/types/process.h>
#include <viua/types/reference.h>
using namespace std;

// Provide storage for static member.
viua::internals::types::register_index const
    viua::process::Process::DEFAULT_REGISTER_SIZE;

viua::kernel::Register* viua::process::Process::register_at(
    viua::internals::types::register_index i,
    viua::internals::Register_sets rs) {
    if (rs == viua::internals::Register_sets::CURRENT) {
        return currently_used_register_set->register_at(i);
    } else if (rs == viua::internals::Register_sets::LOCAL) {
        return stack->back()->local_register_set->register_at(i);
    } else if (rs == viua::internals::Register_sets::STATIC) {
        ensure_static_registers(stack->back()->function_name);
        return static_registers.at(stack->back()->function_name)
            ->register_at(i);
    } else if (rs == viua::internals::Register_sets::GLOBAL) {
        return global_register_set->register_at(i);
    } else {
        throw make_unique<viua::types::Exception>(
            "unsupported register set type");
    }
}

void viua::process::Process::ensure_static_registers(
    std::string function_name) {
    /** Makes sure that static register set for requested function is
     * initialized.
     */
    try {
        static_registers.at(function_name);
    } catch (std::out_of_range const& e) {
        // FIXME: amount of static registers should be customizable
        // FIXME: amount of static registers shouldn't be a magic number
        static_registers[function_name] =
            make_unique<viua::kernel::Register_set>(16);
    }
}

Frame* viua::process::Process::request_new_frame(
    viua::internals::types::register_index arguments_size,
    viua::internals::types::register_index registers_size) {
    return stack->prepare_frame(arguments_size, registers_size);
}
void viua::process::Process::push_frame() {
    if (stack->size() > Stack::MAX_STACK_SIZE) {
        ostringstream oss;
        oss << "stack size (" << Stack::MAX_STACK_SIZE
            << ") exceeded with call to '" << stack->frame_new->function_name
            << '\'';
        throw make_unique<viua::types::Exception>(oss.str());
    }

    currently_used_register_set = stack->frame_new->local_register_set.get();
    if (find(stack->begin(), stack->end(), stack->frame_new) != stack->end()) {
        ostringstream oss;
        oss << "stack corruption: frame " << hex << stack->frame_new.get()
            << dec << " for function " << stack->frame_new->function_name << '/'
            << stack->frame_new->arguments->size() << " pushed more than once";
        throw oss.str();
    }
    stack->emplace_back(std::move(stack->frame_new));
}

auto viua::process::Process::adjust_jump_base_for_block(
    std::string const& call_name) -> viua::internals::types::Op_address_type {
    return stack->adjust_jump_base_for_block(call_name);
}
auto viua::process::Process::adjust_jump_base_for(std::string const& call_name)
    -> viua::internals::types::Op_address_type {
    return stack->adjust_jump_base_for(call_name);
}
auto viua::process::Process::call_native(
    Op_address_type return_address,
    std::string const& call_name,
    viua::kernel::Register* return_register,
    std::string const&) -> Op_address_type {
    auto call_address = adjust_jump_base_for(call_name);

    if (not stack->frame_new) {
        throw make_unique<viua::types::Exception>(
            "function call without a frame: use `frame 0' in source code if "
            "the "
            "function takes no parameters");
    }

    stack->frame_new->function_name   = call_name;
    stack->frame_new->return_address  = return_address;
    stack->frame_new->return_register = return_register;

    push_frame();

    return call_address;
}
auto viua::process::Process::call_foreign(
    Op_address_type return_address,
    std::string const& call_name,
    viua::kernel::Register* return_register,
    std::string const&) -> Op_address_type {
    if (not stack->frame_new) {
        throw make_unique<viua::types::Exception>(
            "external function call without a frame: use `frame 0' in source "
            "code if the function takes no parameters");
    }

    stack->frame_new->function_name   = call_name;
    stack->frame_new->return_address  = return_address;
    stack->frame_new->return_register = return_register;

    suspend();
    scheduler->request_foreign_function_call(stack->frame_new.release(), this);

    return return_address;
}

auto viua::process::Process::push_deferred(std::string const call_name)
    -> void {
    if (not stack->frame_new) {
        throw make_unique<viua::types::Exception>(
            "function call without a frame: use `frame 0' in source code if "
            "the "
            "function takes no parameters");
    }

    stack->frame_new->function_name   = call_name;
    stack->frame_new->return_address  = nullptr;
    stack->frame_new->return_register = nullptr;

    stack->back()->deferred_calls.push_back(std::move(stack->frame_new));
}

void viua::process::Process::handle_active_exception() {
    stack->unwind();
}
auto viua::process::Process::tick() -> Op_address_type {
    Op_address_type previous_instruction_pointer = stack->instruction_pointer;

    try {
        // It is necessary to use a "saved stack" because the stack variable may
        // be changed during the call to dispatch(), and without the saved stack
        // the VM could end up setting instruction pointer of one stack on a
        // different one, thus currupting execution.
        auto saved_stack = stack;

        switch (stack->state_of()) {
        // When stack is in a RUNNING state it can be executed normally with
        // no special conditions.
        case Stack::STATE::RUNNING:
        // When stack is in SUSPENDED_BY_DEFERRED_ON_FRAME_POP state a dispatch
        // should be performed to run the opcode that put the process in this
        // state the second time as such opcodes first suspend the stack, and
        // then continue it when they are entered again.
        case Stack::STATE::SUSPENDED_BY_DEFERRED_ON_FRAME_POP:
            saved_stack->instruction_pointer =
                dispatch(stack->instruction_pointer);
            break;
        case Stack::STATE::UNINITIALISED:
            // Stack should never be uninitialised at runtime.
            break;
        case Stack::STATE::SUSPENDED_BY_DEFERRED_DURING_STACK_UNWINDING:
        case Stack::STATE::HALTED:
            // Not interesting in this case.
            break;
        default:
            break;
        }
    } catch (std::unique_ptr<viua::types::Exception>& e) {
        /*
         * All machine-thrown exceptions are passed back to user code.
         * This is much easier than checking for erroneous conditions and
         * terminating functions conditionally, instead - machine just throws
         * viua::types::Exception objects which are then caught here.
         *
         * If user code cannot deal with them (i.e. did not register a catcher
         * block) they will terminate execution later.
         */
        stack->thrown = std::move(e);
    } catch (std::unique_ptr<viua::types::Value>& e) {
        /*
         * All values can be thrown as exceptions, so Values must also be
         * caught.
         */
        stack->thrown = std::move(e);
    }

    if (stack->state_of() == Stack::STATE::HALTED or stack->size() == 0) {
        finished.store(true, std::memory_order_release);
        return nullptr;
    }

    /*  Machine should halt execution if previous instruction pointer is the
     * same as current one as it means that the execution flow is corrupted and
     *  entered an infinite loop.
     *
     *  However, execution *should not* be halted if:
     *      - the offending opcode is RETURN (as this may indicate exiting
     * recursive function),
     *      - the offending opcode is JOIN (as this means that a process is
     * waiting for another process to finish),
     *      - the offending opcode is RECEIVE (as this means that a process is
     * waiting for a message),
     *      - an object has been thrown, as the instruction pointer will be
     * adjusted by catchers or execution will be halted on unhandled types,
     */
    if (stack->instruction_pointer == previous_instruction_pointer
        and stack->state_of() == viua::process::Stack::STATE::RUNNING
        and (OPCODE(*stack->instruction_pointer) != RETURN
             and OPCODE(*stack->instruction_pointer) != JOIN
             and OPCODE(*stack->instruction_pointer) != RECEIVE)
        and (not stack->thrown)) {
        stack->thrown =
            make_unique<viua::types::Exception>("InstructionUnchanged");
    }

    if (stack->thrown and stack->frame_new) {
        /*  Delete active frame after an exception is thrown.
         *  There're two reasons for such behaviour:
         *  - it prevents memory leaks if an exception escapes and
         *    is handled by the watchdog process,
         *  - if the exception is caught, it provides the servicing block with a
         *    clean environment (as there is no way of dropping a frame without
         *    using it),
         */
        stack->frame_new.reset(nullptr);
    }

    if (stack->thrown
        or (stack->state_of()
            == Stack::STATE::SUSPENDED_BY_DEFERRED_DURING_STACK_UNWINDING)) {
        handle_active_exception();
    }

    if (stack->thrown) {
        return nullptr;
    }

    return stack->instruction_pointer;
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

std::string viua::process::Process::starting_function() const {
    return stack->entry_function;
}

auto viua::process::Process::priority() const -> decltype(process_priority) {
    return process_priority;
}
void viua::process::Process::priority(decltype(process_priority) p) {
    process_priority = p;
}

bool viua::process::Process::stopped() const {
    return (finished.load(std::memory_order_acquire) or terminated());
}

bool viua::process::Process::terminated() const {
    return static_cast<bool>(stack->thrown);
}

void viua::process::Process::pass(std::unique_ptr<viua::types::Value> message) {
    message_queue.push(std::move(message));
    wakeup();
}


viua::types::Value* viua::process::Process::get_active_exception() {
    return stack->thrown.get();
}

std::unique_ptr<viua::types::Value> viua::process::Process::
    transfer_active_exception() {
    return std::move(stack->thrown);
}

void viua::process::Process::raise(
    std::unique_ptr<viua::types::Value> exception) {
    stack->thrown = std::move(exception);
}


std::unique_ptr<viua::types::Value> viua::process::Process::get_return_value() {
    return std::move(stack->return_value);
}

bool viua::process::Process::watchdogged() const {
    return (not watchdog_function.empty());
}
std::string viua::process::Process::watchdog() const {
    return watchdog_function;
}
auto viua::process::Process::become(std::string const& function_name,
                                    std::unique_ptr<Frame> frame_to_use)
    -> Op_address_type {
    if (not scheduler->is_native_function(function_name)) {
        throw make_unique<viua::types::Exception>(
            "process from undefined function: " + function_name);
    }

    stack->clear();
    stack->thrown.reset(nullptr);
    stack->caught.reset(nullptr);
    finished.store(false, std::memory_order_release);

    frame_to_use->function_name = function_name;
    stack->frame_new            = std::move(frame_to_use);

    push_frame();

    return (stack->instruction_pointer = adjust_jump_base_for(function_name));
}

auto viua::process::Process::begin() -> Op_address_type {
    if (not scheduler->is_native_function(stack->at(0)->function_name)) {
        throw make_unique<viua::types::Exception>(
            "process from undefined function: " + stack->at(0)->function_name);
    }
    return (stack->instruction_pointer =
                adjust_jump_base_for(stack->at(0)->function_name));
}
auto viua::process::Process::execution_at() const
    -> decltype(stack->instruction_pointer) {
    return stack->instruction_pointer;
}


std::vector<Frame*> viua::process::Process::trace() const {
    auto tr = std::vector<Frame*>{};
    for (auto& each : *stack) {
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

void viua::process::Process::migrate_to(
    viua::scheduler::Virtual_process_scheduler* sch) {
    scheduler = sch;
}

viua::process::Process::Process(std::unique_ptr<Frame> frm,
                                viua::scheduler::Virtual_process_scheduler* sch,
                                viua::process::Process* pt,
                                bool const enable_tracing)
        : tracing_enabled(enable_tracing)
        , scheduler(sch)
        , parent_process(pt)
        , global_register_set(nullptr)
        , currently_used_register_set(nullptr)
        , stack(nullptr)
        , finished(false)
        , is_joinable(true)
        , is_suspended(false)
        , process_priority(512)
        , process_id(this)
        , is_hidden(false) {
    global_register_set =
        make_unique<viua::kernel::Register_set>(DEFAULT_REGISTER_SIZE);
    currently_used_register_set = frm->local_register_set.get();
    auto s                      = make_unique<Stack>(frm->function_name,
                                this,
                                &currently_used_register_set,
                                global_register_set.get(),
                                scheduler);
    s->emplace_back(std::move(frm));
    s->bind(&currently_used_register_set, global_register_set.get());
    stack           = s.get();
    stacks[s.get()] = std::move(s);
}

viua::process::Process::~Process() {}
