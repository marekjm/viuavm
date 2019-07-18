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

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/io.h>
#include <viua/types/exception.h>
#include <viua/types/io.h>

static auto is_sentinel(std::unique_ptr<viua::types::IO_interaction> const& item) -> bool {
    return (item.get() == nullptr);
}

void viua::scheduler::io::io_scheduler(
    uint64_t const scheduler_id,
    viua::kernel::Kernel& kernel,
    std::deque<std::unique_ptr<viua::types::IO_interaction>>& requests,
    std::mutex& io_request_mutex,
    std::condition_variable& io_request_cv) {

    while (true) {
        std::unique_ptr<viua::types::IO_interaction> work;
        {
            std::unique_lock<std::mutex> lck { io_request_mutex };

            /*
             *             WARNING! ACHTUNG! HERE BE DRAGONS!
             *
             * Do not try to simplify the condition by removing the seemingly
             * redundant `not`s - they are not! With them both removed the condition
             * encodes the same logic, but for some reason the VM will hang.
             * It looks like one thread is monopolising the lock (maybe it is
             * running in too tight a loop?) which leads to starvation of other
             * threads who never get to lock it... including the main kernel thread.
             */
            while (not io_request_cv.wait_for(lck, std::chrono::milliseconds{10}, [&requests]{
                return (not requests.empty());
            }));

            work = std::move(requests.front());
            requests.pop_front();
        }

        if (is_sentinel(work)) {
            if constexpr (false) {
                std::cerr <<
                    ("[io][id=" + std::to_string(scheduler_id) + "] received sentinel\n");
            }
            break;
        }

        using viua::types::IO_interaction;
        auto result = work->interact();
        if (result.state == IO_interaction::State::Complete) {
            kernel.complete_io(work->id(), (
                (result.status == IO_interaction::Status::Success)
                ? viua::kernel::Kernel::IO_result::make_success
                : viua::kernel::Kernel::IO_result::make_error
            )(std::move(result.result)));
        } else {
            kernel.schedule_io(std::move(work));
        }
    }

    std::cerr <<
        ("[io][id=" + std::to_string(scheduler_id) + "] scheduler shutting down\n");
}
