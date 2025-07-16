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

#include <sstream>
#include <string>

#include <viua/arch/arch.h>


namespace viua::arch {
Register_access::Register_access()
    : set{ viua::arch::REGISTER_SET::VOID }
    , index{ 0 }
{}
Register_access::Register_access(
    viua::arch::REGISTER_SET const s,
    register_index_type const i)
    : set{ s }
    , index{ i }
{}
auto Register_access::decode(
    underlying_type const raw) -> Register_access
{
    auto set = static_cast<viua::arch::REGISTER_SET>(
        (raw & REGISTER_ACCESS_SET_MASK) >> 6);
    auto index =
        static_cast<register_index_type>(raw & REGISTER_ACCESS_INDEX_MASK);
    return Register_access{ set, index };
}
auto Register_access::encode() const -> underlying_type
{
    auto base = underlying_type{ index };
    auto rset = static_cast<underlying_type>(set);
    return static_cast<underlying_type>(base | (rset << 6));
}

auto Register_access::make_local(
    register_index_type const index) -> Register_access
{
    return Register_access{ viua::arch::REGISTER_SET::LOCAL, index };
}
auto Register_access::make_argument(
    register_index_type const index) -> Register_access
{
    return Register_access{ viua::arch::REGISTER_SET::ARGUMENT, index };
}
auto Register_access::make_parameter(
    register_index_type const index) -> Register_access
{
    return Register_access{ viua::arch::REGISTER_SET::PARAMETER, index };
}
auto Register_access::make_void() -> Register_access
{
    return Register_access{ viua::arch::REGISTER_SET::VOID, 0 };
}

auto Register_access::to_string() const -> std::string
{
    if (is_void()) {
        return "void";
    }

    auto out = std::ostringstream{};
    out << '$';
    out << static_cast<unsigned int>(index);
    out << '.';
    switch (set) {
        using enum viua::arch::REGISTER_SET;
        case VOID:
            return "void";
        case LOCAL:
            out << 'l';
            break;
        case ARGUMENT:
            out << 'a';
            break;
        case PARAMETER:
            out << 'p';
            break;
    }
    return out.str();
}
}  // namespace viua::arch
