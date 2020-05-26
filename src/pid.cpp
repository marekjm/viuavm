/*
 *  Copyright (C) 2016, 2020 Marek Marecki
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

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <viua/bytecode/maps.h>
#include <viua/bytecode/opcodes.h>
#include <viua/kernel/kernel.h>
#include <viua/process.h>
#include <viua/types/exception.h>
#include <viua/types/integer.h>
#include <viua/types/process.h>
#include <viua/types/reference.h>


viua::process::PID::PID(pid_type const p)
    : value{p}
{}
auto viua::process::PID::operator<(PID const& other) const -> bool
{
    /*
     * PIDs can't really have a less-than relation they are either equal or not,
     * and that's it. The less-than relation is implemented only so that
     * viua::process::PID values may be used as keys in std::map<>.
     */
    return (value < other.value);
}
auto viua::process::PID::operator==(PID const& other) const -> bool
{
    return (value == other.value);
}
auto viua::process::PID::operator!=(PID const& other) const -> bool
{
    return (value != other.value);
}

auto viua::process::PID::get() const -> pid_type
{
    return value;
}

auto viua::process::PID::str(bool const readable) const -> std::string
{
    constexpr auto SEPARATOR = ':';

    auto s = std::ostringstream{};
    {
        auto const [base, big, small, n, m] = value;
        s << std::hex << std::setw(16) << std::setfill('0') << base;
        if (readable) { s << SEPARATOR; }
        s << std::hex << std::setw(16) << std::setfill('0') << big;
        if (readable) { s << SEPARATOR; }
        s << std::hex << std::setw(8) << std::setfill('0') << small;
        if (readable) { s << SEPARATOR; }
        s << std::hex << std::setw(4) << std::setfill('0') << n;
        if (readable) { s << SEPARATOR; }
        s << std::hex << std::setw(4) << std::setfill('0') << m;
    }
    return s.str();
}

viua::process::Pid_emitter::Pid_emitter()
{
    std::random_device rd;

    base = std::uniform_int_distribution<uint64_t>{
        0, static_cast<uint64_t>(-1)
    }(rd);

    auto const b_high = (std::uniform_int_distribution<uint64_t>{
        0, static_cast<uint16_t>(-1)
    }(rd) << 48);
    auto const b_low = std::uniform_int_distribution<uint64_t>{
        0, static_cast<uint16_t>(-1)
    }(rd);
    auto const b_full = (b_high + b_low);
    big_offset = b_full;

    auto const s_high = (std::uniform_int_distribution<uint32_t>{
        0, 0x0fff
    }(rd) << 20);
    auto const s_low = std::uniform_int_distribution<uint32_t>{
        0, static_cast<uint8_t>(-1)
    }(rd);
    auto const s_full = (s_high + s_low);
    small_offset = s_full;

    /*
     * The "small" component has modulo 2 and there is no reason we should have
     * more PID fields increasing with the same frequency. This is why they have
     * mininum modulo values set a little bit higher. Note that both N and M
     * components may still end up having the same modulo due to randomness.
     */
    constexpr auto N_MIN_MODULO = uint16_t{3};
    constexpr auto M_MIN_MODULO = uint8_t{4};

    n_offset = std::uniform_int_distribution<uint16_t>{
        0, static_cast<uint16_t>(-1)
    }(rd);
    n_modulo = std::uniform_int_distribution<uint8_t>{
        N_MIN_MODULO, static_cast<uint8_t>(-1)
    }(rd);

    m_offset = std::uniform_int_distribution<uint16_t>{
        0, static_cast<uint8_t>(-1)
    }(rd);
    m_modulo = std::uniform_int_distribution<uint8_t>{
        M_MIN_MODULO, static_cast<uint8_t>(-1)
    }(rd);
}

auto viua::process::Pid_emitter::emit() -> PID::pid_type
{
    ++big;

    if ((big % 2) == 0) {
        ++small;
    }

    if ((big % n_modulo) == 0) {
        ++n;
    }
    if ((small % m_modulo) == 0) {
        ++m;
    }

    return PID::pid_type{
          base
        , (big_offset + big)
        , (small_offset + small)
        , (n_offset + n)
        , (m_offset + m)
    };
}
