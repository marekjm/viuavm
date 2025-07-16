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

#include <endian.h>
#include <stdio.h>
#include <string.h>

#include <print>

#include <viua/arch/arch.h>
#include <viua/vm/ins.h>


namespace viua::vm::ins {
using namespace viua::arch::ins;
using viua::vm::Stack;
using ip_type = viua::arch::instruction_type const*;

/*
 * SM - Store Memory
 */
auto execute(
    SM const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const base = immutable_proxy(stack, op.instruction.in)
                          .get<register_type::pointer_type>();

    auto const unit      = op.instruction.spec;
    auto const copy_size = (1u << unit);
    auto const offset    = (op.instruction.immediate * copy_size);

    if (not base.has_value()) {
        throw abort_execution{ stack,
                               "invalid base operand for memory instruction" };
    }

    auto const pointer_info = stack.proc->get_pointer(base->ptr);
    if (not pointer_info.has_value()) {
        auto o = std::ostringstream{};
        o << "unknown pointer: ";
        o << std::hex << std::setfill('0') << std::setw(16) << base->ptr;
        throw abort_execution{ stack, o.str() };
    }

    if (offset >= pointer_info->size) {
        auto o = std::ostringstream{};
        o << "illegal offset of " << offset << " bytes into a region of "
          << pointer_info->size << " byte(s)";
        throw abort_execution{ stack, o.str() };
    }
    if ((offset + copy_size) > pointer_info->size) {
        auto o = std::ostringstream{};
        o << "illegal store of " << copy_size << " bytes into a region of "
          << (pointer_info->size - offset) << " byte(s)";
        throw abort_execution{ stack, o.str() };
    }

    auto const user_addr = base->ptr + offset;
    auto const addr      = stack.proc->memory_at(user_addr);
    if (addr == nullptr) {
        auto o = std::ostringstream{};
        o << "invalid store address: ";
        o << std::hex << std::setfill('0') << std::setw(16) << user_addr;
        throw abort_execution{ stack, o.str() };
    }

    auto const in = immutable_proxy(stack, op.instruction.out);
    if (in.holds<void>()) {
        throw abort_execution{ stack,
                               "invalid in operand for memory instruction" };
    }
    auto const buf = in.target.as_memory();
    memcpy(addr, buf.data(), copy_size);
}

/*
 * LM - Load Memory
 */
auto execute(
    LM const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const base = immutable_proxy(stack, op.instruction.in)
                          .get<register_type::pointer_type>();

    auto const unit      = op.instruction.spec;
    auto const copy_size = (1u << unit);
    auto const offset    = (op.instruction.immediate * copy_size);

    if (not base.has_value()) {
        throw abort_execution{ stack,
                               "invalid base operand for memory instruction" };
    }

    auto const pointer_info = stack.proc->get_pointer(base->ptr);
    if (not pointer_info.has_value()) {
        auto o = std::ostringstream{};
        o << "unknown pointer: ";
        o << std::hex << std::setfill('0') << std::setw(16) << base->ptr;
        throw abort_execution{ stack, o.str() };
    }

    auto const ffi_pointer = pointer_info->foreign;
    if ((not ffi_pointer) and offset >= pointer_info->size) {
        auto o = std::ostringstream{};
        o << "illegal offset of " << offset << " bytes into a region of "
          << pointer_info->size << " byte(s)";
        throw abort_execution{ stack, o.str() };
    }
    if ((not ffi_pointer) and (offset + copy_size) > pointer_info->size) {
        auto o = std::ostringstream{};
        o << "illegal load of " << copy_size << " bytes from a region of "
          << (pointer_info->size - offset) << " byte(s)";
        throw abort_execution{ stack, o.str() };
    }

    auto const user_addr = base->ptr + offset;
    auto const addr      = ffi_pointer ? reinterpret_cast<uint8_t*>(base->ptr)
                                       : stack.proc->memory_at(user_addr);
    if (addr == nullptr) {
        auto o = std::ostringstream{};
        o << "invalid load address: ";
        o << std::hex << std::setfill('0') << std::setw(16) << user_addr;
        throw abort_execution{ stack, o.str() };
    }

    auto out = mutable_proxy(stack, op.instruction.out);
    auto raw = register_type::undefined_type{};
    memcpy(raw.data(), addr, copy_size);
    out                         = raw;
    out.to()->get().loaded_size = copy_size;
}

/*
 * AA - Allocate Automatic
 */
auto execute(
    AA const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const base = immutable_proxy(stack, op.instruction.in).get<uint64_t>();
    auto const alignment = (1u << op.instruction.spec);

    if (not base.has_value()) {
        throw abort_execution{ stack,
                               "invalid base operand type in "
                                   + op.instruction.in.to_string()
                                   + " for aa instruction" };
    }

    // FIXME Ensure that enough memory is available to satisfy both size and
    // alignment request.
    auto size = (*base * alignment);

    stack.proc->stack_break -= size;
    stack.frames.back().saved.sbrk = stack.proc->stack_break;
    auto const pointer_address     = stack.proc->stack_break;

    mutable_proxy(stack, op.instruction.out) =
        register_type::pointer_type{ pointer_address };

    auto pointer_info = Pointer{};
    pointer_info.ptr  = pointer_address;
    pointer_info.size = size;
    stack.proc->record_pointer(pointer_info);

    // FIXME check if there is enough memory to accomodate size
    memset(stack.proc->memory_at(pointer_address), 0, size);
}

/*
 * AD - Allocate Dynamic
 */
auto execute(
    AD const,
    Stack&,
    ip_type const) -> void
{}

/*
 * PTR - PoinTeR
 */
auto execute(
    PTR const,
    Stack&,
    ip_type const) -> void
{}

auto execute(
    ARODP const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const& strtab     = *stack.proc->strtab;
    auto const data_offset = op.instruction.immediate;

    auto const pointer_address = const_cast<uint8_t*>(&strtab[data_offset]);
    mutable_proxy(stack, op.instruction.out) = register_type::pointer_type{
        reinterpret_cast<uintptr_t>(pointer_address)
    };

    auto pointer_info    = Pointer{};
    pointer_info.ptr     = reinterpret_cast<uintptr_t>(pointer_address);
    pointer_info.foreign = true;
    stack.proc->record_pointer(pointer_info);
}
auto execute(
    ATXTP const op,
    Stack& stack,
    ip_type const) -> void
{
    mutable_proxy(stack, op.instruction.out) =
        register_type::pointer_type{ op.instruction.immediate };
}
}  // namespace viua::vm::ins
