/*
 *  Copyright (C) 2019 Marek Marecki
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

#include <viua/machine.h>
#include <viua/kernel/kernel.h>
#include <viua/process.h>
#include <viua/types/vector.h>
#include <viua/types/string.h>
#include <viua/scheduler/process.h>

namespace viua { namespace scheduler {
Process_scheduler::Process_scheduler(viua::kernel::Kernel& k):
    attached_kernel{k}
{}

Process_scheduler::~Process_scheduler() {}

auto Process_scheduler::bootstrap(std::vector<std::string> args) -> void {
    auto initial_frame           = std::make_unique<Frame>(nullptr, 0);
    initial_frame->function_name = ENTRY_FUNCTION_NAME;
    initial_frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(2));

    auto cmdline = std::make_unique<viua::types::Vector>();
    for (auto const& each : args) {
        cmdline->push(std::make_unique<viua::types::String>(each));
    }
    initial_frame->local_register_set->set(1, std::move(cmdline));

    main_process = spawn(std::move(initial_frame), nullptr, true);
    main_process->priority(16);
}

auto Process_scheduler::spawn(std::unique_ptr<Frame> frame, process_type* parent, bool disown)
    -> viua::process::Process* {
    auto process = std::make_unique<process_type>(
        std::move(frame)
        , this
        , parent
        , false /* tracing_enabled */
    );

    process->start();
    if (disown) {
        process->detach();
    }

    auto pid_of_new_process = process->pid();
    attached_kernel.create_mailbox(pid_of_new_process);
    if (not disown) {
        attached_kernel.create_result_slot_for(pid_of_new_process);
    }

    auto process_ptr = process.get();
    process_queue.emplace_back(std::move(process));

    return process_ptr;
}

auto Process_scheduler::is_joinable(viua::process::PID const pid) const -> bool {
    return attached_kernel.is_process_joinable(pid);
}
auto Process_scheduler::is_stopped(viua::process::PID const pid) const -> bool {
    return attached_kernel.is_process_stopped(pid);
}
auto Process_scheduler::is_terminated(viua::process::PID const pid) const -> bool {
    return attached_kernel.is_process_terminated(pid);
}

auto Process_scheduler::transfer_exception_of(viua::process::PID const pid) const -> std::unique_ptr<viua::types::Value> {
    return attached_kernel.transfer_exception_of(pid);
}
auto Process_scheduler::transfer_result_of(viua::process::PID const pid) const -> std::unique_ptr<viua::types::Value> {
    return attached_kernel.transfer_result_of(pid);
}

auto Process_scheduler::send(viua::process::PID const pid, std::unique_ptr<viua::types::Value> message) -> void {
    attached_kernel.send(pid, std::move(message));
}
auto Process_scheduler::receive(viua::process::PID const pid,
             std::queue<std::unique_ptr<viua::types::Value>>& message_queue) -> void {
    attached_kernel.receive(pid, message_queue);
}

auto Process_scheduler::request_ffi_call(std::unique_ptr<Frame> frame, viua::process::Process& p) -> void {
    attached_kernel.request_foreign_function_call(std::move(frame), p);
}

auto Process_scheduler::is_native_function(std::string const name) const -> bool {
    return attached_kernel.is_native_function(name);
}
auto Process_scheduler::is_foreign_function(std::string const name) const -> bool {
    return attached_kernel.is_foreign_function(name);
}
auto Process_scheduler::is_block(std::string const name) const -> bool {
    return attached_kernel.is_block(name);
}

auto Process_scheduler::load_module(std::string const name) -> void {
    attached_kernel.load_module(name);
}


auto Process_scheduler::get_entry_point_of_block(std::string const name) const
    -> std::pair<viua::internals::types::Op_address_type,
                 viua::internals::types::Op_address_type> {
    return attached_kernel.get_entry_point_of_block(name);
}
auto Process_scheduler::get_entry_point_of_function(std::string const& name) const
    -> std::pair<viua::internals::types::Op_address_type,
                 viua::internals::types::Op_address_type> {
    return attached_kernel.get_entry_point_of(name);
}

auto Process_scheduler::launch() -> void {
}
auto Process_scheduler::shutdown() -> void {
}
auto Process_scheduler::join() -> void {
}
auto Process_scheduler::exit() const -> int {
    return 0;
}
}}
