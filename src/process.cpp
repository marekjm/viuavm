/*
 *  Copyright (C) 2015-2020 Marek Marecki
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
#include <viua/scheduler/process.h>
#include <viua/types/exception.h>
#include <viua/types/integer.h>
#include <viua/types/io.h>
#include <viua/types/pointer.h>
#include <viua/types/process.h>
#include <viua/types/reference.h>


auto viua::process::Decoder_adapter::fetch_slot(Op_address_type& addr) const
    -> viua::bytecode::codec::Register_access
{
    auto [next_addr, reg] = decoder.decode_register(addr);

    addr = next_addr;

    return reg;
}

auto viua::process::Decoder_adapter::fetch_register(
    Op_address_type& addr,
    Process& proc,
    bool const pointers_allowed) const -> viua::kernel::Register*
{
    auto const reg = fetch_slot(addr);

    using viua::bytecode::codec::Access_specifier;

    if ((not pointers_allowed)
        and (std::get<2>(reg) == Access_specifier::Pointer_dereference)) {
        // This should never happen - assembler should catch such situations.
        throw std::make_unique<viua::types::Exception>(
            viua::types::Exception::Tag{"Invalid_access"},
            "pointer dereference not allowed");
    }

    auto index = std::get<1>(reg);
    auto slot  = proc.register_at(
        index,
        static_cast<viua::bytecode::codec::Register_set>(std::get<0>(reg)));

    if (std::get<2>(reg) == Access_specifier::Register_indirect) {
        auto const i =
            static_cast<viua::types::Integer*>(slot->get())->as_integer();
        if (i < 0) {
            throw std::make_unique<viua::types::Exception>(
                viua::types::Exception::Tag{"Invalid_register_index"},
                "registers cannot be negative");
        }
        return proc.register_at(
            static_cast<viua::bytecode::codec::register_index_type>(i),
            static_cast<viua::bytecode::codec::Register_set>(std::get<0>(reg)));
    }

    return slot;
}

auto viua::process::Decoder_adapter::fetch_register_or_void(
    Op_address_type& addr,
    Process& proc,
    bool const pointers_allowed) const -> std::optional<viua::kernel::Register*>
{
    auto const ot = static_cast<OperandType>(*addr);
    if (ot == OT_VOID) {
        ++addr;
        return {};
    }

    return fetch_register(addr, proc, pointers_allowed);
}

auto viua::process::Decoder_adapter::fetch_tagged_register(
    Op_address_type& addr,
    Process& proc,
    bool const pointers_allowed) const
    -> std::pair<viua::bytecode::codec::Register_set, viua::kernel::Register*>
{
    auto const reg = fetch_slot(addr);

    using viua::bytecode::codec::Access_specifier;

    if ((not pointers_allowed)
        and (std::get<2>(reg) == Access_specifier::Pointer_dereference)) {
        // This should never happen - assembler should catch such situations.
        throw std::make_unique<viua::types::Exception>(
            viua::types::Exception::Tag{"Invalid_access"},
            "pointer dereference not allowed");
    }

    auto index = std::get<1>(reg);
    auto slot  = proc.register_at(
        index,
        static_cast<viua::bytecode::codec::Register_set>(std::get<0>(reg)));

    if (std::get<2>(reg) == Access_specifier::Register_indirect) {
        auto const i =
            static_cast<viua::types::Integer*>(slot->get())->as_integer();
        if (i < 0) {
            throw std::make_unique<viua::types::Exception>(
                viua::types::Exception::Tag{"Invalid_register_index"},
                "registers cannot be negative");
        }
        return {
            static_cast<viua::bytecode::codec::Register_set>(std::get<0>(reg)),
            proc.register_at(
                static_cast<viua::bytecode::codec::register_index_type>(i),
                static_cast<viua::bytecode::codec::Register_set>(
                    std::get<0>(reg)))};
    }

    return {static_cast<viua::bytecode::codec::Register_set>(std::get<0>(reg)),
            slot};
}

auto viua::process::Decoder_adapter::fetch_register_index(
    Op_address_type& addr) const -> viua::bytecode::codec::register_index_type
{
    return std::get<1>(fetch_slot(addr));
}

auto viua::process::Decoder_adapter::fetch_tagged_register_index(
    Op_address_type& addr) const
    -> std::pair<viua::bytecode::codec::Register_set,
                 viua::bytecode::codec::register_index_type>
{
    auto const slot = fetch_slot(addr);
    return {static_cast<viua::bytecode::codec::Register_set>(std::get<0>(slot)),
            std::get<1>(slot)};
}

auto viua::process::Decoder_adapter::fetch_value(Op_address_type& addr,
                                                 Process& proc) const
    -> viua::types::Value*
{
    auto [next_addr, reg] = decoder.decode_register(addr);

    addr = next_addr;

    auto slot = proc.register_at(std::get<1>(reg), std::get<0>(reg));

    using viua::bytecode::codec::Access_specifier;

    if (std::get<2>(reg) == Access_specifier::Register_indirect) {
        auto const i =
            static_cast<viua::types::Integer*>(slot->get())->as_integer();
        if (i < 0) {
            throw std::make_unique<viua::types::Exception>(
                viua::types::Exception::Tag{"Invalid_register_index"},
                "registers cannot be negative");
        }
        slot = proc.register_at(
            static_cast<viua::bytecode::codec::register_index_type>(i),
            std::get<0>(reg));
    }

    auto value = slot->get();
    if (not value) {
        throw std::make_unique<viua::types::Exception>(
            "read from null register: " + std::to_string(std::get<1>(reg)));
    }

    if (auto ref = dynamic_cast<viua::types::Reference*>(value)) {
        value = ref->points_to();
    }
    if (std::get<2>(reg) == Access_specifier::Pointer_dereference) {
        auto const pointer = dynamic_cast<viua::types::Pointer*>(value);
        if (pointer == nullptr) {
            throw std::make_unique<viua::types::Exception>(
                viua::types::Exception::Tag{"Not_a_pointer"},
                "dereferenced value is not a pointer: " + value->type());
        }
        value = pointer->to(proc);
    }
    if (auto pointer = dynamic_cast<viua::types::Pointer*>(value)) {
        pointer->authenticate(proc.pid());
    }

    return value;
}

auto viua::process::Decoder_adapter::fetch_string(Op_address_type& addr) const
    -> std::string
{
    auto [next_addr, s] = decoder.decode_string(addr);

    addr = next_addr;

    return s;
}

auto viua::process::Decoder_adapter::fetch_bits_string(
    Op_address_type& addr) const -> std::vector<uint8_t>
{
    auto [next_addr, s] = decoder.decode_bits_string(addr);

    addr = next_addr;

    return s;
}

auto viua::process::Decoder_adapter::fetch_timeout(Op_address_type& addr) const
    -> viua::bytecode::codec::timeout_type
{
    auto [next_addr, t] = decoder.decode_timeout(addr);

    addr = next_addr;

    return t;
}

auto viua::process::Decoder_adapter::fetch_float(Op_address_type& addr) const
    -> double
{
    auto [next_addr, v] = decoder.decode_f64(addr);

    addr = next_addr;

    return v;
}

auto viua::process::Decoder_adapter::fetch_i32(Op_address_type& addr) const
    -> int32_t
{
    auto [next_addr, v] = decoder.decode_i32(addr);

    addr = next_addr;

    return v;
}

auto viua::process::Decoder_adapter::fetch_bool(Op_address_type& addr) const
    -> bool
{
    auto [next_addr, v] = decoder.decode_bool(addr);

    addr = next_addr;

    return v;
}

auto viua::process::Decoder_adapter::fetch_address(Op_address_type& addr) const
    -> uint64_t
{
    auto [next_addr, v] = decoder.decode_address(addr);

    addr = next_addr;

    return v;
}

auto viua::process::Process::register_at(
    viua::bytecode::codec::register_index_type i,
    viua::bytecode::codec::Register_set rs) -> viua::kernel::Register*
{
    if (rs == viua::bytecode::codec::Register_set::Local) {
        return stack->back()->local_register_set->register_at(i);
    } else if (rs == viua::bytecode::codec::Register_set::Static) {
        ensure_static_registers(stack->back()->function_name);
        return static_registers.at(stack->back()->function_name)
            ->register_at(i);
    } else if (rs == viua::bytecode::codec::Register_set::Global) {
        return global_register_set->register_at(i);
    } else if (rs == viua::bytecode::codec::Register_set::Parameters) {
        return stack->back()->arguments->register_at(i);
    } else if (rs == viua::bytecode::codec::Register_set::Arguments) {
        return stack->frame_new->arguments->register_at(i);
    } else {
        throw std::make_unique<viua::types::Exception>(
            "unsupported register set type");
    }
}

auto viua::process::Process::ensure_static_registers(std::string function_name)
    -> void
{
    /** Makes sure that static register set for requested function is
     * initialized.
     */
    try {
        static_registers.at(function_name);
    } catch (std::out_of_range const& e) {
        // FIXME: amount of static registers should be customizable
        // FIXME: amount of static registers shouldn't be a magic number
        static_registers[function_name] =
            std::make_unique<viua::kernel::Register_set>(16);
    }
}

auto viua::process::Process::request_new_frame(
    viua::bytecode::codec::register_index_type const arguments_size) -> Frame*
{
    return stack->prepare_frame(arguments_size);
}
auto viua::process::Process::push_frame() -> void
{
    if (stack->size() > Stack::MAX_STACK_SIZE) {
        std::ostringstream oss;
        oss << "stack size (" << Stack::MAX_STACK_SIZE
            << ") exceeded with call to '" << stack->frame_new->function_name
            << '\'';
        throw std::make_unique<viua::types::Exception>(oss.str());
    }

    if (find(stack->begin(), stack->end(), stack->frame_new) != stack->end()) {
        std::ostringstream oss;
        oss << "stack corruption: frame " << std::hex << stack->frame_new.get()
            << std::dec << " for function " << stack->frame_new->function_name
            << '/' << stack->frame_new->arguments->size()
            << " pushed more than once";
        throw oss.str();
    }
    stack->emplace_back(std::move(stack->frame_new));
}

auto viua::process::Process::adjust_jump_base_for_block(
    std::string const& call_name) -> viua::internals::types::Op_address_type
{
    return stack->adjust_jump_base_for_block(call_name);
}
auto viua::process::Process::adjust_jump_base_for(std::string const& call_name)
    -> viua::internals::types::Op_address_type
{
    return stack->adjust_jump_base_for(call_name);
}
auto viua::process::Process::call_native(
    Op_address_type return_address,
    std::string const& call_name,
    viua::kernel::Register* return_register,
    std::string const&) -> Op_address_type
{
    auto call_address = adjust_jump_base_for(call_name);

    if (not stack->frame_new) {
        throw std::make_unique<viua::types::Exception>(
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
    std::string const&) -> Op_address_type
{
    if (not stack->frame_new) {
        throw std::make_unique<viua::types::Exception>(
            "function call without a frame: use `frame 0' in source code if "
            "the "
            "function takes no parameters");
    }

    stack->frame_new->function_name   = call_name;
    stack->frame_new->return_address  = return_address;
    stack->frame_new->return_register = return_register;

    suspend();
    attached_scheduler->request_ffi_call(std::move(stack->frame_new), *this);

    return return_address;
}

auto viua::process::Process::push_deferred(std::string const call_name) -> void
{
    if (not stack->frame_new) {
        throw std::make_unique<viua::types::Exception>(
            "function call without a frame: use `frame 0' in source code if "
            "the "
            "function takes no parameters");
    }

    stack->frame_new->function_name   = call_name;
    stack->frame_new->return_address  = nullptr;
    stack->frame_new->return_register = nullptr;

    stack->back()->deferred_calls.push_back(std::move(stack->frame_new));
}

auto viua::process::Process::handle_active_exception() -> void
{
    stack->unwind();
}
auto viua::process::Process::tick() -> Op_address_type
{
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
         * All machine-thrown exceptions are passed back to user code. This is
         * much easier than checking for erroneous conditions and terminating
         * functions conditionally, instead - machine just throws
         * viua::types::Exception objects which are then caught here.
         *
         * If user code cannot deal with them (i.e. did not register a catcher
         * block) they will terminate execution later.
         *
         * It is impossible for bytecode to throw something that is not a proper
         * exception; the 'throw' instruction always wraps values in exceptions.
         */
        raise(std::move(e));
    }

    if (stack->state_of() == Stack::STATE::HALTED or stack->size() == 0) {
        finished.store(true, std::memory_order_release);
        return nullptr;
    }

    if (suspended()) {
        return stack->instruction_pointer;
    }

    /*
     * Machine should halt execution if previous instruction pointer is the same
     * as current one as it means that the execution flow is corrupted and
     * entered an infinite loop.
     *
     * However, execution *should not* be halted if:
     * - the offending opcode is RETURN (as this may indicate exiting recursive
     *   function)
     * - the offending opcode is JOIN (as this means that a process is waiting
     *   for another process to finish)
     * - the offending opcode is RECEIVE (as this means that a process is
     *   waiting for a message)
     * - the offending opcode is IO_WAIT (as this means that a process is
     *   waiting for I/O to complete)
     * - an object has been thrown, as the instruction pointer will be adjusted
     *   by catchers or execution will be halted on unhandled types
     */
    auto const allowed_unchanged_ops = std::set<OPCODE>{
        RETURN,
        JOIN,
        RECEIVE,
        IO_WAIT,
    };
    if (stack->instruction_pointer == previous_instruction_pointer
        and stack->state_of() == viua::process::Stack::STATE::RUNNING
        and (not allowed_unchanged_ops.count(
            OPCODE(*stack->instruction_pointer)))
        and (not stack->thrown)) {
        auto tp = std::vector<viua::types::Exception::Throw_point>{};
        tp.push_back(viua::types::Exception::Throw_point{
            reinterpret_cast<uint64_t>(current_stack().jump_base),
            static_cast<uint64_t>(current_stack().instruction_pointer
                                  - current_stack().jump_base)});
        stack->thrown = std::make_unique<viua::types::Exception>(
            tp, viua::types::Exception::Tag{"InstructionUnchanged"});
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

auto viua::process::Process::join() -> void
{
    /** Join a process with calling process.
     *
     *  This function causes calling process to be blocked until
     *  this process has stopped.
     */
    is_joinable.store(false, std::memory_order_release);
}
auto viua::process::Process::detach() -> void
{
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
auto viua::process::Process::joinable() const -> bool
{
    return is_joinable.load(std::memory_order_acquire);
}

auto viua::process::Process::suspend() -> void
{
    is_suspended.store(true, std::memory_order_release);
}
auto viua::process::Process::wakeup() -> void
{
    is_suspended.store(false, std::memory_order_release);
}
auto viua::process::Process::suspended() const -> bool
{
    return is_suspended.load(std::memory_order_acquire);
}

auto viua::process::Process::parent() const -> viua::process::Process*
{
    return parent_process;
}

auto viua::process::Process::starting_function() const -> std::string
{
    return stack->entry_function;
}

auto viua::process::Process::priority() const -> decltype(process_priority)
{
    return process_priority;
}
auto viua::process::Process::priority(decltype(process_priority) p) -> void
{
    process_priority = p;
}

auto viua::process::Process::stopped() const -> bool
{
    return (finished.load(std::memory_order_acquire) or terminated());
}

auto viua::process::Process::terminated() const -> bool
{
    return static_cast<bool>(stack->thrown);
}

auto viua::process::Process::pass(std::unique_ptr<viua::types::Value> message)
    -> void
{
    message_queue.push(std::move(message));
    wakeup();
}


auto viua::process::Process::get_active_exception() -> viua::types::Value*
{
    return stack->thrown.get();
}

auto viua::process::Process::transfer_active_exception()
    -> std::unique_ptr<viua::types::Value>
{
    return std::move(stack->thrown);
}

auto viua::process::Process::raise(
    std::unique_ptr<viua::types::Exception> exception) -> void
{
    using Throw_point = viua::types::Exception::Throw_point;
    exception->add_throw_point(
        Throw_point{reinterpret_cast<uint64_t>(current_stack().jump_base),
                    static_cast<uint64_t>(current_stack().instruction_pointer
                                          - current_stack().jump_base)});
    stack->thrown = std::move(exception);
}
auto viua::process::Process::raise(
    std::unique_ptr<viua::types::Value> exception) -> void
{
    // FIXME remove the Value overload of Process::raise()
    raise(std::make_unique<viua::types::Exception>(std::move(exception)));
}


auto viua::process::Process::get_return_value()
    -> std::unique_ptr<viua::types::Value>
{
    return std::move(stack->return_value);
}

auto viua::process::Process::watchdogged() const -> bool
{
    return (not watchdog_function.empty());
}
auto viua::process::Process::watchdog() const -> std::string
{
    return watchdog_function;
}
auto viua::process::Process::frame_for_watchdog() -> std::unique_ptr<Frame>
{
    return std::move(watchdog_frame);
}

auto viua::process::Process::become(std::string const& function_name,
                                    std::unique_ptr<Frame> frame_to_use)
    -> Op_address_type
{
    if (not attached_scheduler->is_native_function(function_name)) {
        throw std::make_unique<viua::types::Exception>(
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

auto viua::process::Process::start() -> Op_address_type
{
    if (not attached_scheduler->is_native_function(
            stack->at(0)->function_name)) {
        throw std::make_unique<viua::types::Exception>(
            "process from undefined function: " + stack->at(0)->function_name);
    }
    return (stack->instruction_pointer =
                adjust_jump_base_for(stack->at(0)->function_name));
}
auto viua::process::Process::execution_at() const
    -> decltype(stack->instruction_pointer)
{
    return stack->instruction_pointer;
}


auto viua::process::Process::trace() const -> Trace_type
{
    auto tr = std::vector<Frame const*>{};
    tr.reserve(stack->size());
    for (auto const& each : *stack) {
        tr.push_back(each.get());
    }
    return tr;
}
auto viua::process::Process::depth() const -> Trace_type::size_type
{
    return stack->size();
}

auto viua::process::Process::pid() const -> viua::process::PID
{
    return process_id;
}

auto viua::process::Process::empty() const -> bool
{
    return message_queue.empty();
}

auto viua::process::Process::schedule_io(
    std::unique_ptr<viua::scheduler::io::IO_interaction> i) -> void
{
    attached_scheduler->schedule_io(std::move(i));
}
auto viua::process::Process::cancel_io(
    std::tuple<uint64_t, uint64_t> const interaction_id) -> void
{
    attached_scheduler->cancel_io(interaction_id);
}

auto viua::process::Process::migrate_to(viua::scheduler::Process_scheduler* sch)
    -> void
{
    attached_scheduler = sch;
}

auto viua::process::Process::get_kernel() const -> viua::kernel::Kernel&
{
    return attached_scheduler->kernel();
}

namespace viua::process {
auto Process::attach_pointer(viua::types::Value* const val,
                             viua::types::Pointer* const) -> void
{
    /*
     * At one point the 'live_pointers' contained a set of pointers attached to
     * the value, providing full and accurate tracking of which pointers were
     * attached to which values.
     *
     * This proved to be too brittle for real-world code, though, so the the
     * scope was reduced to just record live values that had a pointer taken.
     *
     * See comment to Process::detach_pointer() for more information and
     * background.
     */
    live_pointers.insert(val);
}
auto Process::detach_pointer(viua::types::Value* const,
                             viua::types::Pointer* const) -> void
{
    /*
     * At one point the VM was accurately tracking the relationships between
     * values and their pointers. This function was used to detach a pointer
     * from a value; a detached pointer would be destroyed soon after, and the
     * value would not mark it as expired when it would be later deleted itself.
     *
     * However, this nice machinery proved to be brittle and was the cause of
     * heap corruption in more complex programs (first example being the
     * self-hosting version of Viuact compiler). And so, this function was made
     * to be a no-op, and left as a placeholder for a future (better)
     * implementation of pointer tracking.
     */
}
auto Process::invalidate_pointers_of(viua::types::Value* const val) -> void
{
    live_pointers.erase(val);
}
auto Process::verify_liveness(viua::types::Pointer& ptr) const -> bool
{
    auto const val = ptr.of();

    if (not live_pointers.count(val)) {
        ptr.expire();
        return false;
    }
    return true;
}
auto Process::verify_liveness(viua::types::Pointer const& ptr) const -> bool
{
    auto const val = ptr.of();

    if (not live_pointers.count(val)) {
        return false;
    }
    return true;
}
}  // namespace viua::process

viua::process::Process::Process(std::unique_ptr<Frame> frm,
                                viua::process::PID const p,
                                viua::scheduler::Process_scheduler* sch,
                                viua::process::Process* pt,
                                bool const enable_tracing,
                                viua::process::Decoder_adapter const& da)
        : tracing_enabled{enable_tracing}
        , attached_scheduler{sch}
        , decoder{da}
        , parent_process{pt}
        , global_register_set{nullptr}
        , stack{nullptr}
        , finished{false}
        , is_joinable{true}
        , is_suspended{false}
        , process_priority{512}
        , process_id{p}
{
    global_register_set =
        std::make_unique<viua::kernel::Register_set>(DEFAULT_REGISTER_SIZE);
    auto s = std::make_unique<Stack>(frm->function_name,
                                     this,
                                     global_register_set.get(),
                                     attached_scheduler);
    s->emplace_back(std::move(frm));
    s->bind(global_register_set.get());
    stack           = s.get();
    stacks[s.get()] = std::move(s);
}

viua::process::Process::~Process()
{}
