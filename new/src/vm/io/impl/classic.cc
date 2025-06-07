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


#include <viua/support/errno.h>
#include <viua/support/print.hh>
#include <viua/vm/core.h>
#include <viua/vm/io/impl/classic.hh>


namespace viua::vm::io::impl::classic {
IO::IO()
{}

IO::~IO()
{}

auto IO::schedule(
    int const fd,
    IO_request::Opcode const opcode,
    io::Out buffer) -> IO_request::id_type
{
    auto const req_id = next_id.fetch_add(1);
    auto req          = std::make_unique<IO_request>(
        fd, nullptr, req_id, opcode, std::move(buffer));

    // FIXME try to immediately satisfy the request

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

    // FIXME try to immediately satisfy the request

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

    auto& req = *requests.at(want_id);

    /*
     * If the request is in flight we should see if we can satisfy it: let us
     * do whatever would be necessary to satisfy the request eg, read some data
     * from a file descriptor if it is a read request.
     *
     * REMEMBER to ALWAYS use NON-BLOCKING I/O to avoid choking the virtual
     * machine (no kink shaming, but we want the machine to run--and you must be
     * able to breathe freely if you are running).
     */
    if (req.status == IO_request::Status::In_flight) {
        switch (req.opcode) {
            using enum IO_request::Opcode;
            case Read:
                {
                    auto& buf               = std::get<io::In>(req.buffer);
                    auto const head         = buf.head();
                    auto const wanted_bytes = buf.remaining();

                    auto const rv = read(req.fd, head, wanted_bytes);

                    if (rv == -1) {
                        req.status = IO_request::Status::Error;
                        break;
                    }

                    buf.consume(static_cast<size_t>(rv));

                    /*
                     * Would be nice to only support whole operations (both on
                     * the read and write side), but we do not live in a perfect
                     * world.
                     *
                     * Sometimes satisfying a whole operation would be
                     * impossible, and what should we do then? One way to solve
                     * this would be to let the user decide and:
                     *
                     *  - issue an io_cancel after a timeout, to cancel the
                     * entire request
                     *  - issue an io_short after a timeout, to allow the
                     * request to be short a few bytes
                     *
                     * Or maybe the VM should return "success" status for whole
                     * results and "short" status short results? In any case,
                     * the user would always have to be prepared to handle short
                     * results. The VM gives the user enough information right
                     * now, by returning the number of bytes transferred (read
                     * or written).
                     *
                     * Let us keep the "worse" ie, allowing short results, API
                     * for now. A flag field can be added later, to let users
                     * adjust the treatment of an I/O request.
                     *
                     * However, I think I like an API that guarantees whole
                     * results (shown in pseudocode):
                     *
                     *      ; prepare a request
                     *      io_submit ...
                     *
                     *      ; wait until it is ready, or cancel if it failed to
                     * meet ; the deadline try { io_wait ..., $timeout } catch
                     * (TIMEOUT) { io_cancel ...
                     *      }
                     *
                     *      ; wait until it is ready, or allow short if it
                     * failed to ; meet the deadline try { io_wait ..., $timeout
                     *      } catch (TIMEOUT) {
                     *          io_short ...
                     *          io_wait ..., void  ; void because it will return
                     *                             ; immediately now that it is
                     *                             ; short
                     *      }
                     *
                     * Well... now that I have written the code above I think it
                     * is suboptimal. It would be much better to decide what do
                     * I want to accept before I wait for the completion of an
                     * I/O operation:
                     *
                     *      req.flags = req.flags | IO_ALLOW_SHORT;
                     *      io_wait $req, $timeout
                     *
                     * Then, if IO_ALLOW_SHORT is enabled io_wait returns a
                     * short result; otherwise it returns nothing ie, an empty
                     * pointer, and the user is responsible for retrying.
                     *
                     * See RWF_NOWAIT flag in readv(2) for inspiration for
                     * IO_ALLOW_SHORT. See the following thread on Hacker News
                     * to learn about whole vs short results in the io_uring API
                     * on Linux: https://news.ycombinator.com/item?id=23134737
                     * Since io_uring is the inspiration for the I/O API of the
                     * VM this is highly relevant.
                     */
                    if constexpr (false) {
                        if (buf.empty()) {
                            req.status = IO_request::Status::Success;
                        }
                    }
                    if ((rv > 0) or buf.empty()) {
                        req.status = IO_request::Status::Success;
                    }

                    break;
                }
            case Write:
                {
                    auto& buf               = std::get<io::Out>(req.buffer);
                    auto const head         = buf.head();
                    auto const wanted_bytes = buf.remaining();

                    auto const rv = write(req.fd, head, wanted_bytes);
                    if (rv == -1) {
                        req.status = IO_request::Status::Error;
                        break;
                    }

                    buf.consume(static_cast<size_t>(rv));

                    /*
                     * See the comment above for Read operations. For writes we
                     * can afford to pretend we guarantee whole results, because
                     * no test actually checks it.
                     */
                    if (buf.empty()) {
                        req.status = IO_request::Status::Success;
                    }

                    break;
                }
        }
    }

    if (req.status == IO_request::Status::In_flight) {
        return nullptr;
    }

    if (req.status == IO_request::Status::Success) {
        switch (req.opcode) {
            using enum IO_request::Opcode;
            case Read:
                {
                    auto const size_ptr =
                        stack.proc->memory_at(
                            reinterpret_cast<uint64_t>(req.req_ptr))
                        + (sizeof(uint64_t) * 2);
                    auto const result_size =
                        std::get<io::In>(req.buffer).consumed();
                    static_assert(sizeof(result_size) == sizeof(uint64_t));
                    memcpy(size_ptr, &result_size, sizeof(uint64_t));
                    break;
                }
            case Write:
                {
                    /* ignore */
                    break;
                }
        }
    }

    auto const req_ptr = req.req_ptr;
    requests.erase(want_id);
    return req_ptr;
}
}  // namespace viua::vm::io::impl::classic
