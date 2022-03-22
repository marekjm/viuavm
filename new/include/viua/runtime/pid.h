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

#ifndef VIUA_PID_H
#define VIUA_PID_H

#include <netinet/in.h>
#include <stdint.h>
#include <sys/socket.h>

#include <string>


namespace viua::runtime {
class Process;

struct PID {
    using pid_type = in6_addr;

  private:
    pid_type const value;

  public:
    auto operator<=>(PID const&) const -> std::strong_ordering;

    auto get() const -> pid_type;
    auto to_string() const -> std::string;

    explicit PID(pid_type const);
};

class Pid_emitter {
    in6_addr base{};
    uint64_t counter{};

  public:
    Pid_emitter();

    auto emit() -> PID;
};
}  // namespace viua::runtime


#endif
