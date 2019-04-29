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
}}
