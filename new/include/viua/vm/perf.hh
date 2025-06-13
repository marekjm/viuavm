/*
 *  Copyright (C) 2021-2022 Marek Marecki
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

#ifndef VIUA_VM_CORE_PERF_HH
#define VIUA_VM_CORE_PERF_HH

#include <stdint.h>

#include <chrono>


namespace viua::vm {
struct Performance_counters {
    using counter_type = uint64_t;
    counter_type total_ops_executed{ 0 };
    counter_type total_us_elapsed{ 0 };

    using time_point_type = std::chrono::time_point<std::chrono::steady_clock>;
    time_point_type bang{};
    time_point_type death{};

    inline auto start() -> void
    {
        bang = std::chrono::steady_clock::now();
    }
    inline auto stop() -> void
    {
        death = std::chrono::steady_clock::now();
    }
    inline auto duration() const -> auto
    {
        return (death - bang);
    }
};
}  // namespace viua::vm

#endif
