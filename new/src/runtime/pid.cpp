/*
 *  Copyright (C) 2016, 2020, 2022 Marek Marecki
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

#include <arpa/inet.h>
#include <endian.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <array>
#include <random>
#include <stdexcept>
#include <string>

#include <viua/runtime/pid.h>


namespace viua::runtime {
PID::PID(pid_type const p) : value{p}
{}

auto PID::operator<=>(PID const& other) const -> std::strong_ordering
{
    /*
     * PIDs can't really have a less-than relation they are either equal or not,
     * and that's it. The less-than relation is implemented only so that
     * PID values may be used as keys in std::map<>.
     */
    auto const lhs = reinterpret_cast<unsigned char const*>(value.s6_addr);
    auto const rhs =
        reinterpret_cast<unsigned char const*>(other.value.s6_addr);
    for (auto i = size_t{0}; i < sizeof(decltype(value)::s6_addr); ++i) {
        if (auto cmp = (lhs[i] <=> rhs[i]); cmp != 0) {
            return cmp;
        }
    }
    return std::strong_ordering::equal;
}

auto PID::get() const -> pid_type
{
    return value;
}

auto PID::to_string() const -> std::string
{
    std::array<char, INET6_ADDRSTRLEN> buf{'\0'};
    inet_ntop(AF_INET6, &value, buf.data(), buf.size());
    return "[" + std::string{buf.data()} + "]";
}

Pid_emitter::Pid_emitter() : base{{{0xfe, 0x80, 0x00}}}, counter{0}
{
    if (auto seed = getenv("VIUA_VM_PID_SEED"); seed != nullptr) {
        if (inet_pton(AF_INET6, seed, &base) == 0) {
            throw std::invalid_argument{
                "VIUA_VM_PID_SEED must contain an IPv6 address"};
        }

        memcpy(&counter, base.s6_addr + 8, sizeof(counter));
        counter = be64toh(counter);
    } else {
        std::random_device rd;
        counter = std::uniform_int_distribution<uint64_t>{
            0, static_cast<uint64_t>(-1)}(rd);
    }
}

auto Pid_emitter::emit() -> PID
{
    auto c = htobe64(counter++);
    auto p = base;

    memcpy(p.s6_addr + 8, &c, sizeof(c));

    return PID{p};
}

}  // namespace viua::runtime
