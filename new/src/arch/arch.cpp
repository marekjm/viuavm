/*
 *  Copyright (C) 2021 Marek Marecki
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

#include <viua/arch/arch.h>
#include <viua/arch/ops.h>


namespace viua::arch {
Register_access::Register_access()
        : set{viua::arch::REGISTER_SET::VOID}, direct{true}, index{0}
{}
Register_access::Register_access(viua::arch::REGISTER_SET const s,
                                 bool const d,
                                 uint8_t const i)
        : set{s}, direct{d}, index{i}
{}
auto Register_access::decode(uint16_t const raw) -> Register_access
{
    auto set    = static_cast<viua::arch::REGISTER_SET>((raw & 0x0e00) >> 9);
    auto direct = static_cast<bool>(raw & 0x0100);
    auto index  = static_cast<uint8_t>(raw & 0x00ff);
    return Register_access{set, direct, index};
}
auto Register_access::encode() const -> uint16_t
{
    auto base = uint16_t{index};
    auto mode = static_cast<uint16_t>(direct);
    auto rset = static_cast<uint16_t>(set);
    return base | (mode << 8) | (rset << 9);
}

auto Register_access::make_local(uint8_t const index, bool const direct)
    -> Register_access
{
    return Register_access{viua::arch::REGISTER_SET::LOCAL, direct, index};
}
auto Register_access::make_void() -> Register_access
{
    return Register_access{viua::arch::REGISTER_SET::VOID, true, 0};
}
}  // namespace viua::arch
