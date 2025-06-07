/*
 *  Copyright (C) 2021-2025 Marek Marecki
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

#ifndef VIUA_VM_CORE_IO_SCHED_HH
#define VIUA_VM_CORE_IO_SCHED_HH

#include <stdint.h>

#include <atomic>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <variant>


namespace viua::vm {
namespace io {
/*
 * An out-buffer is used for write requests (or other request types that do not
 * need to store any data back into process memory). It represents a chunk of
 * memory that will only be read by the request it is attached to; data that
 * will be PUSHED OUT OF memory:
 *
 *      .--------.        .-------.
 *      | memory | -----> | WORLD |
 *      '--------'        '-------'
 */
struct Out {
    using buffer_type = std::span<uint8_t>;
    buffer_type buffer;
    size_t consumed_bytes{ 0 };

    explicit inline Out(
        buffer_type&& v)
        : buffer{ std::move(v) }
    {}

    /*
     * Return the first byte that still has to be read from the buffer.
     * The data() function is an alias for head(), to make it compatible with
     * standard containers.
     */
    auto head() const -> uint8_t const*;
    inline auto data() const -> uint8_t const*
    {
        return head();
    }

    /*
     * Return the count of the bytes remaining to be read from the buffer.
     */
    auto remaining() const -> size_t;

    /*
     * If/When all bytes were consumed, the buffer becomes empty.
     */
    auto empty() const -> bool;

    /*
     * The initial size of the buffer.
     */
    auto size() const -> size_t;

    /*
     * Consume a portion of bytes, moving head of the buffer forward and
     * reducing the amount of remaining bytes. The memory the buffer is pointing
     * to is read-only, but the buffer view itself is mutable as it needs to
     * keep track of what data has been used already.
     */
    auto consume(size_t const) -> void;

    /*
     * The number of consumed bytes. See the comment about RWF_NOWAIT and short
     * results in classic I/O backend. Since we need to support short results
     * there must be a way to see how many bytes were actually consumed by an
     * operation.
     */
    auto consumed() const -> size_t;
};

/*
 * An in-buffer is used for read requests (or other request types that need to
 * store data in process memory). It represents a chunk of memory that will be
 * written into by the request it is attached to; data that will be PULLED INTO
 * memory:
 *
 *      .--------.        .-------.
 *      | memory | <----- | WORLD |
 *      '--------'        '-------'
 *
 * All member functions perform actions analogous to their counterparts in the
 * out-buffer type.
 */
struct In {
    using buffer_type = std::span<uint8_t, std::dynamic_extent>;
    buffer_type buffer;
    size_t consumed_bytes{ 0 };

    explicit inline In(
        buffer_type&& v)
        : buffer{ std::move(v) }
    {}

    auto head() -> uint8_t*;
    inline auto data() -> uint8_t*
    {
        return head();
    }
    auto remaining() const -> size_t;
    auto empty() const -> bool;
    auto size() const -> size_t;
    auto consume(size_t const) -> void;
    auto consumed() const -> size_t;
};
}  // namespace io

struct IO_request {
    using id_type = uint64_t;
    id_type const id{};

    using fd_type = int;
    fd_type const fd;

    /*
     * The underlying type should match the type of the `opcode' field of the
     * io_uring_sqe struct.
     */
    enum class Opcode : uint8_t
    {
        Read,
        Write,
    };
    Opcode const opcode{};

    std::variant<io::Out, io::In> buffer;

    /*
     * Points to the request descriptor WITHIN the memory allocated for an
     * actor, and is used to RETURN data to the process.
     *
     * ALWAYS launder this address through Process::memory_at().
     */
    uint8_t* const req_ptr{ nullptr };

    enum class Status
    {
        In_flight,
        Executing,
        Success,
        Error,
        Cancel,
    };
    Status status{ Status::In_flight };

    inline IO_request(
        fd_type const d,
        uint8_t* const rp,
        id_type const i,
        Opcode const o,
        io::In b)
        : id{ i }
        , fd{ d }
        , opcode{ o }
        , buffer{ b }
        , req_ptr{ rp }
    {}
    inline IO_request(
        fd_type const d,
        uint8_t* const rp,
        id_type const i,
        Opcode const o,
        io::Out b)
        : id{ i }
        , fd{ d }
        , opcode{ o }
        , buffer{ b }
        , req_ptr{ rp }
    {}
};

struct Stack;

struct IO_scheduler {
    using id_type = std::atomic<IO_request::id_type>;
    id_type next_id;

    using map_type =
        std::unordered_map<IO_request::id_type, std::unique_ptr<IO_request>>;
    map_type requests;

    virtual ~IO_scheduler();

    /*
     * Opcode type must not be wider than the opcode field of the io_uring_seq
     * struct. See io_uring(7).
     */
    using opcode_type = uint8_t;

    /*
     * The programming model used for I/O on Viua is simple:
     *
     *  - you prepare an operation by setting the target file descriptor, the
     *    opcode, and the buffer and its size, in the operation description
     *  - you io_submit the operation to the kernel and get an ID back
     *  - the kernel executes the operation in the background, while you are
     *    free to do something else
     *  - you io_wait for the operation with a given ID and get a result if the
     *    operation has already completed; otherwise you get nothing back from
     *    the kernel as an indication to try again later
     *
     * However, some not all operating system APIs match this model. Some
     * require a file descriptor to be registered first, and then you can watch
     * for events happening on that file descriptor.
     *
     * The watch() function serves this purpose: to let the I/O scheduler know
     * it should take an interest in the file descriptor. Some schedulers may
     * ignore it, if their underlying API uses a model similar to the one
     * employed by Viua.
     */
    virtual auto watch(int const) -> void;

    /*
     * The schedule() functions implement the io_submit part of the API.
     */
    virtual auto schedule(int const, IO_request::Opcode const, io::Out)
        -> IO_request::id_type                           = 0;
    virtual auto schedule(uint8_t* const,
                          int const,
                          IO_request::Opcode const,
                          io::In) -> IO_request::id_type = 0;

    /*
     * The wait() function implements the io_wait part of the API.
     */
    virtual auto wait(Stack&, IO_request::id_type const) -> uint8_t* = 0;
};
}  // namespace viua::vm

#endif
