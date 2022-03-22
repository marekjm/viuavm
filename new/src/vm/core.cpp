/*
 *  Copyright (C) 2021-2022 Marek Marecki
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

#include <memory>

#include <viua/vm/core.h>


namespace viua::vm {
auto IO_scheduler::schedule(int const fd,
                            opcode_type const opcode,
                            buffer_type buffer) -> IO_request::id_type
{
    auto const req_id = next_id.fetch_add(1);
    auto req = std::make_unique<IO_request>(req_id, opcode, std::move(buffer));

    io_uring_sqe* sqe{};
    sqe = io_uring_get_sqe(&ring);

    sqe->opcode = opcode;
    sqe->fd     = fd;
    sqe->addr =
        reinterpret_cast<decltype(io_uring_sqe::addr)>(req->buffer.data());
    sqe->len       = req->buffer.size();
    sqe->user_data = req_id;

    io_uring_submit(&ring);

    requests[req_id] = std::move(req);

    return req_id;
}

auto Core::find(pid_type const p) -> std::experimental::observer_ptr<Process>
{
    using std::experimental::make_observer;
    return flock.count(p)
        ? make_observer<Process>(flock.at(p).get())
        : nullptr;
}

auto Core::spawn(std::string mod_name, uint64_t const entry) -> pid_type
{
    auto const& mod = modules.at(mod_name);

    auto const pid = pids.emit();
    auto proc      = std::make_unique<Process>(pid, this, mod);
    proc->push_frame(256, (mod.ip_base + entry), nullptr);

    run_queue.push(std::experimental::make_observer<Process>(proc.get()));
    flock.insert({ pid, std::move(proc) });

    return pid;
}
}  // namespace viua::vm
