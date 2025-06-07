/*
 *  Copyright (C) 2025 Marek Marecki
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


#include <viua/vm/core.h>
#include <viua/vm/io/impl/io_uring.hh>

namespace viua::vm::io::impl::io_uring {
IO::IO()
{
    io_uring_queue_init(IO_URING_ENTRIES, &ring, 0);
}

IO::~IO()
{
    io_uring_queue_exit(&ring);
}

auto IO::schedule(
    int const fd,
    IO_request::Opcode const opcode,
    io::Out buffer) -> IO_request::id_type
{
    auto const req_id = next_id.fetch_add(1);
    auto req          = std::make_unique<IO_request>(
        fd, nullptr, req_id, opcode, std::move(buffer));

    io_uring_sqe* sqe{};
    sqe = io_uring_get_sqe(&ring);

    switch (opcode) {
        using enum IO_request::Opcode;
        case Read:
            sqe->opcode = IORING_OP_READ;
            break;
        case Write:
            sqe->opcode = IORING_OP_WRITE;
            break;
    }
    sqe->fd = fd;

    auto& buf      = std::get<decltype(buffer)>(req->buffer);
    sqe->addr      = reinterpret_cast<decltype(io_uring_sqe::addr)>(buf.data());
    sqe->len       = buf.size();
    sqe->user_data = req_id;

    io_uring_submit(&ring);

    requests[req_id] = std::move(req);

    return req_id;
}
auto IO::schedule(
    uint8_t* const req_ptr,
    int const fd,
    IO_request::Opcode const opcode,
    io::In buffer) -> IO_request::id_type
{
    auto const req_id = next_id.fetch_add(1);
    auto req          = std::make_unique<IO_request>(
        fd, req_ptr, req_id, opcode, std::move(buffer));

    io_uring_sqe* sqe{};
    sqe = io_uring_get_sqe(&ring);

    switch (opcode) {
        using enum IO_request::Opcode;
        case Read:
            sqe->opcode = IORING_OP_READ;
            break;
        case Write:
            sqe->opcode = IORING_OP_WRITE;
            break;
    }
    sqe->fd = fd;

    auto& buf      = std::get<decltype(buffer)>(req->buffer);
    sqe->addr      = reinterpret_cast<decltype(io_uring_sqe::addr)>(buf.data());
    sqe->len       = buf.size();
    sqe->user_data = req_id;

    io_uring_submit(&ring);

    requests[req_id] = std::move(req);

    return req_id;
}

auto IO::wait(
    Stack& stack,
    IO_request::id_type const want_id) -> uint8_t*
{
    if (not requests.contains(want_id)) {
        return nullptr;
    }

    io_uring_cqe* cqe{};
    do {
        io_uring_wait_cqe(&ring, &cqe);

        if (cqe->res == -1) {
            requests[cqe->user_data]->status = IO_request::Status::Error;
        } else {
            auto& rd  = *requests[cqe->user_data];
            rd.status = IO_request::Status::Success;

            switch (rd.opcode) {
                case IO_request::Opcode::Read:
                    {
                        auto const size_ptr =
                            stack.proc->memory_at(
                                reinterpret_cast<uint64_t>(rd.req_ptr))
                            + (sizeof(uint64_t) * 2);
                        auto const buffer_size = htole64(cqe->res);
                        memcpy(size_ptr, &buffer_size, sizeof(buffer_size));
                        break;
                    }
                case IO_request::Opcode::Write:
                    {
                        /* ignore */
                        break;
                    }
                default:
                    throw abort_execution{ stack, "invalid I/O opcode" };
            }
        }

        io_uring_cqe_seen(&ring, cqe);
    } while (cqe->user_data != want_id);

    auto const req_ptr = requests[cqe->user_data]->req_ptr;
    requests.erase(want_id);
    return req_ptr;
}
}  // namespace viua::vm::io::impl::io_uring
