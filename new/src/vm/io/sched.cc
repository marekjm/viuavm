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

#include <viua/vm/core.h>


namespace viua::vm::io {
auto Out::head() const -> uint8_t const*
{
    return buffer.data() + consumed_bytes;
}

auto Out::remaining() const -> size_t
{
    return size() - consumed_bytes;
}

auto Out::empty() const -> bool
{
    return size() == consumed_bytes;
}

auto Out::size() const -> size_t
{
    return buffer.size();
}

auto Out::consume(
    size_t const v) -> void
{
    auto const eaten = consumed_bytes + v;
    if (eaten > size()) {
        abort();
    }

    consumed_bytes = eaten;
}

auto Out::consumed() const -> size_t
{
    return consumed_bytes;
}
}  // namespace viua::vm::io


namespace viua::vm::io {
auto In::head() -> uint8_t*
{
    return buffer.data() + consumed_bytes;
}

auto In::remaining() const -> size_t
{
    return size() - consumed_bytes;
}

auto In::empty() const -> bool
{
    return size() == consumed_bytes;
}

auto In::size() const -> size_t
{
    return buffer.size();
}

auto In::consume(
    size_t const v) -> void
{
    auto const eaten = consumed_bytes + v;
    if (eaten > size()) {
        abort();
    }

    consumed_bytes = eaten;
}

auto In::consumed() const -> size_t
{
    return consumed_bytes;
}
}  // namespace viua::vm::io


namespace viua::vm {
IO_scheduler::~IO_scheduler()
{}

auto IO_scheduler::watch(
    int const) -> void
{
    /*
     * Do nothing.
     */
}
}  // namespace viua::vm
