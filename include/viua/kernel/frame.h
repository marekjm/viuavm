/*
 *  Copyright (C) 2015, 2016 Marek Marecki
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

#ifndef VIUA_CPU_FRAME_H
#define VIUA_CPU_FRAME_H

#include <memory>
#include <string>
#include <vector>
#include <viua/bytecode/bytetypedef.h>
#include <viua/kernel/registerset.h>
#include <viua/util/memory.h>

class Frame {
  public:
    uint8_t const* return_address;
    std::unique_ptr<viua::kernel::Register_set> arguments;
    viua::util::memory::maybe_unique_ptr<viua::kernel::Register_set>
        local_register_set;

    viua::kernel::Register* return_register;

    std::vector<std::unique_ptr<Frame>> deferred_calls;

    std::string function_name;

    inline auto ret_address() const -> uint8_t const*
    {
        return return_address;
    }

    auto set_local_register_set(std::unique_ptr<viua::kernel::Register_set>)
        -> void;
    auto set_local_register_set(viua::kernel::Register_set* const,
                                bool const receives_ownership = true) -> void;

    Frame(uint8_t const* const,
          viua::bytecode::codec::register_index_type const);
    Frame(Frame const&) = delete;
    Frame(Frame&&)      = delete;
    auto operator=(Frame const&) = delete;
    auto operator=(Frame&&) = delete;
};


#endif
