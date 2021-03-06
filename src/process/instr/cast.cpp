/*
 *  Copyright (C) 2015-2020 Marek Marecki
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
#include <string>

#include <viua/bytecode/bytetypedef.h>
#include <viua/kernel/kernel.h>
#include <viua/types/exception.h>
#include <viua/types/float.h>
#include <viua/types/integer.h>
#include <viua/types/string.h>
#include <viua/types/value.h>


auto viua::process::Process::opitof(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto const source =
        decoder.fetch_value_of<viua::types::Integer>(addr, *this);

    *target = std::make_unique<viua::types::Float>(source->as_float());

    return addr;
}

auto viua::process::Process::opftoi(Op_address_type addr) -> Op_address_type
{
    auto target       = decoder.fetch_register(addr, *this);
    auto const source = decoder.fetch_value_of<viua::types::Float>(addr, *this);

    *target = std::make_unique<viua::types::Integer>(source->as_integer());

    return addr;
}

auto viua::process::Process::opstoi(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto const source =
        decoder.fetch_value_of<viua::types::String>(addr, *this);

    auto result_integer        = int{0};
    auto const supplied_string = source->value();
    try {
        result_integer = std::stoi(supplied_string);
    } catch (std::out_of_range const& e) {
        throw std::make_unique<viua::types::Exception>("out of range for stoi: "
                                                       + supplied_string);
    } catch (std::invalid_argument const& e) {
        throw std::make_unique<viua::types::Exception>(
            "invalid argument for stoi: " + supplied_string);
    }

    *target = std::make_unique<viua::types::Integer>(result_integer);

    return addr;
}

auto viua::process::Process::opstof(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto const source =
        decoder.fetch_value_of<viua::types::String>(addr, *this);

    auto const supplied_string = source->value();
    try {
        *target =
            std::make_unique<viua::types::Float>(std::stod(supplied_string));
    } catch (std::out_of_range const& e) {
        throw std::make_unique<viua::types::Exception>("out of range for stof: "
                                                       + supplied_string);
    } catch (std::invalid_argument const& e) {
        throw std::make_unique<viua::types::Exception>(
            "invalid argument for stof: " + supplied_string);
    }

    return addr;
}
