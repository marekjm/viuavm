/*
 *  Copyright (C) 2015, 2016, 2018 Marek Marecki
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

#include <memory>

#include <viua/kernel/frame.h>


auto Frame::set_local_register_set(
    std::unique_ptr<viua::kernel::Register_set> rs) -> void
{
    local_register_set.reset(rs.release(), true);  // FIXME use std::move()
                                                   // instead of .release()
}

auto Frame::set_local_register_set(viua::kernel::Register_set* const rs,
                                   bool const receives_ownership) -> void
{
    local_register_set.reset(rs, receives_ownership);
}

Frame::Frame(uint8_t const* const ra,
             viua::bytecode::codec::register_index_type const argsize)
        : return_address{ra}
        , arguments{nullptr}
        , local_register_set{nullptr}
        , return_register{nullptr}
{
    arguments = std::make_unique<viua::kernel::Register_set>(argsize);
}
