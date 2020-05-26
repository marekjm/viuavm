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

#ifndef VIUA_PID_H
#define VIUA_PID_H

#include <string>


namespace viua { namespace process {
class Process;

struct PID {
    using pid_type = std::tuple<
          uint64_t  // base:  chosen randomly at VM start
        , uint64_t  // big:   increasing sequentially; begins with a randomly chosen
                    //        highest and lowest 16 bits
        , uint32_t  // small: increasing sequentially; begins with a randomly chosen
                    //        highest 12 and lowest 8 bits
        , uint16_t  // n:     increasing sequentially every big N (randomly chosen at VM
                    //        start from [0; 2^8)); begins with random [0; 2^16)
        , uint16_t  // m:     increasing sequentially every small M (randomly chosen at VM
                    //        start); begins with random [0; 2^8)
    >;

  private:
    pid_type const value;

  public:
    auto operator<(PID const&) const -> bool;
    auto operator==(PID const&) const -> bool;
    auto operator!=(PID const&) const -> bool;

    auto get() const -> pid_type;
    auto str(bool const readable = true) const -> std::string;

    PID(pid_type const);
};

class Pid_emitter {
    // 1st field
    uint64_t base = 0;

    // 2nd field
    uint64_t big_offset = 0;
    uint64_t big = 0;

    // 3rd field
    uint32_t small_offset = 0;
    uint32_t small = 0;

    // 4th field
    uint8_t n_modulo = 1;
    uint16_t n_offset = 0;
    uint16_t n = 0;

    // 5th field
    uint8_t m_modulo = 1;
    uint16_t m_offset = 0;
    uint16_t m = 0;

  public:
    Pid_emitter();

    auto emit() -> PID::pid_type;
};
}}  // namespace viua::process


#endif
