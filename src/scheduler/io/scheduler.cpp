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
#include <viua/scheduler/io/interactions.h>
#include <viua/types/exception.h>
#include <viua/types/io.h>

static auto is_sentinel(
    std::unique_ptr<viua::scheduler::io::IO_interaction> const& item) -> bool
{
    return (item.get() == nullptr);
}

void viua::scheduler::io::io_scheduler(
    uint64_t const scheduler_id,
    viua::kernel::Kernel& kernel,
    std::deque<std::unique_ptr<IO_interaction>>& requests,
    std::mutex& io_request_mutex,
    std::condition_variable& io_request_cv)
{
    auto local_interactions = std::deque<std::unique_ptr<IO_interaction>>{};

    while (true) {
        std::unique_ptr<IO_interaction> interaction;
        {
            std::unique_lock<std::mutex> lck{io_request_mutex};

            /*
             *             WARNING! ACHTUNG! HERE BE DRAGONS!
             *
             * Do not try to simplify the condition by removing the seemingly
             * redundant `not`s - they are not! With them both removed the
             * condition encodes the same logic, but for some reason the VM will
             * hang. It looks like one thread is monopolising the lock (maybe it
             * is running in too tight a loop?) which leads to starvation of
             * other threads who never get to lock it... including the main
             * kernel thread.
             */
            auto const timeout =
                std::chrono::milliseconds{local_interactions.empty() ? 10 : 0};
            while (not io_request_cv.wait_for(
                lck, timeout, [&requests, &local_interactions] {
                    return (
                        not(requests.empty() and local_interactions.empty()));
                }))
                ;

            interaction = std::move(requests.front());
            requests.pop_front();
        }

        if (is_sentinel(interaction)) {
            if constexpr ((false)) {
                std::cerr << ("[io][id=" + std::to_string(scheduler_id)
                              + "] received sentinel\n");
            }
            break;
        }

        if (not interaction->fd().has_value()) {
            /*
             * Do not work with interactions that do not expose file
             * descriptors. Just fail them. Maybe next time they will behave
             * better.
             */
            kernel.complete_io(
                interaction->id(),
                viua::kernel::Kernel::IO_result::make_error(
                    std::make_unique<viua::types::Exception>(
                        "IO_without_fd",
                        "I/O port did not expose file descriptor")));
            continue;
        }

        {
            auto& work = *interaction;

            fd_set readfds;
            fd_set writefds;
            FD_ZERO(&readfds);
            FD_ZERO(&writefds);

            auto always_ready = false;

            switch (work.kind()) {
            case IO_kind::Input:
                FD_SET(*work.fd(), &readfds);
                break;
            case IO_kind::Output:
                FD_SET(*work.fd(), &writefds);
                break;
            case IO_kind::Close:
                always_ready = true;
                break;
            default:
                /*
                 * Just set it for both modes, to be on the safe side.
                 */
                FD_SET(*work.fd(), &readfds);
                FD_SET(*work.fd(), &writefds);
                break;
            }

            timeval timeout;
            memset(&timeout, 0, sizeof(timeout));
            timeout.tv_sec  = 0;
            timeout.tv_usec = 1;

            auto const s =
                select(*work.fd() + 1, &readfds, &writefds, nullptr, &timeout);
            if (s == -1) {
                auto const saved_errno = errno;
                if constexpr ((false)) {
                    std::cerr << ("[io][id=" + std::to_string(scheduler_id)
                                  + "] select(3) error: "
                                  + std::to_string(saved_errno) + "\n");
                }
                kernel.schedule_io(std::move(interaction));
                continue;
            }
            if (s == 0 and (not always_ready)) {
                if constexpr ((false)) {
                    std::cerr << ("[io][id=" + std::to_string(scheduler_id)
                                  + "] select(3) returned 0 for "
                                  + (work.kind() == IO_kind::Input ? "input"
                                                                   : "output")
                                  + " interaction on fd "
                                  + std::to_string(*work.fd()) + "\n");
                }
                kernel.schedule_io(std::move(interaction));
                continue;
            }

            auto is_ready = false;
            switch (work.kind()) {
            case IO_kind::Input:
                is_ready = FD_ISSET(*work.fd(), &readfds);
                break;
            case IO_kind::Output:
                is_ready = FD_ISSET(*work.fd(), &writefds);
                break;
            case IO_kind::Close:
                is_ready = true;
                break;
            default:
                /* It is safe to do nothing in this case. */
                break;
            }

            if (not is_ready) {
                kernel.schedule_io(std::move(interaction));
                continue;
            }

            auto result = work.interact();
            if (result.state == IO_interaction::State::Complete) {
                kernel.complete_io(
                    work.id(),
                    ((result.status == IO_interaction::Status::Success)
                         ? viua::kernel::Kernel::IO_result::make_success
                         : viua::kernel::Kernel::IO_result::make_error)(
                        std::move(result.result)));
            }
            else {
                kernel.schedule_io(std::move(interaction));
            }
        }
    }

    if constexpr ((false)) {
        std::cerr << ("[io][id=" + std::to_string(scheduler_id)
                      + "] scheduler shutting down\n");
    }
}
