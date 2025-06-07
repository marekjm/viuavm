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

#include <string.h>

#include <viua/arch/arch.h>
#include <viua/support/fdstream.h>
#include <viua/vm/ins.h>


namespace viua {
extern viua::support::fdstream TRACE_STREAM;
}

namespace viua::vm::ins {
using namespace viua::arch::ins;
using viua::vm::Stack;

auto execute(
    IO_SUBMIT const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const dst     = mutable_proxy(stack, op.instruction.out);
    auto const io_desc = immutable_proxy(stack, op.instruction.lhs);

    if (not io_desc.holds<register_type::pointer_type>()) {
        throw abort_execution{ stack, "invalid I/O request description" };
    }

    auto const req_ptr_raw = io_desc.get<register_type::pointer_type>()->ptr;
    auto const req_ptr     = stack.proc->memory_at(req_ptr_raw);

    auto io_op = uint16_t{};
    memcpy(&io_op, req_ptr, sizeof(io_op));
    io_op = le16toh(io_op);

    auto io_port = uint64_t{};
    memcpy(&io_port, req_ptr + (sizeof(uint64_t) * 1), sizeof(io_port));
    io_port = le64toh(io_port);

    auto const size_ptr = req_ptr + (sizeof(uint64_t) * 2);
    auto buffer_size    = uint64_t{ 0 };
    memcpy(&buffer_size, size_ptr, sizeof(buffer_size));
    buffer_size = le64toh(buffer_size);

    auto data_ptr_raw = uintptr_t{ 0 };
    memcpy(
        &data_ptr_raw, req_ptr + (sizeof(uint64_t) * 3), sizeof(data_ptr_raw));
    auto const data_ptr = stack.proc->memory_at(data_ptr_raw);

    switch (io_op) {
        case 0:
            {
                auto buffer =
                    io::In{ io::In::buffer_type{ data_ptr, buffer_size } };
                auto const rd = stack.proc->core->io.schedule(
                    reinterpret_cast<uint8_t*>(req_ptr_raw),
                    io_port,
                    IO_request::Opcode::Read,
                    std::move(buffer));
                dst = rd;
                break;
            }
        case 1:
            {
                auto buffer =
                    io::Out{ io::Out::buffer_type{ data_ptr, buffer_size } };
                auto const rd = stack.proc->core->io.schedule(
                    io_port, IO_request::Opcode::Write, std::move(buffer));
                dst = rd;
                break;
            }
    }
}
auto execute(
    IO_WAIT const op,
    Stack& stack,
    ip_type const) -> void
{
    auto dst = mutable_proxy(stack, op.instruction.out);
    auto req = mutable_proxy(stack, op.instruction.lhs);

    if (not req.holds<uint64_t>()) {
        throw abort_execution{ stack, "invalid I/O request ID" };
    }

    auto const want_id = *req.get<uint64_t>();
    auto const req_ptr = stack.proc->core->io.wait(stack, want_id);
    dst = register_type::pointer_type{ reinterpret_cast<uint64_t>(req_ptr) };
}
auto execute(
    IO_SHUTDOWN const,
    Stack&,
    ip_type const) -> void
{}
auto execute(
    IO_CTL const,
    Stack&,
    ip_type const) -> void
{}
auto execute(
    IO_PEEK const,
    Stack&,
    ip_type const) -> void
{}
}  // namespace viua::vm::ins
