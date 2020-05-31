/*
 *  Copyright (C) 2017, 2018, 2020 Marek Marecki
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

#include <viua/types/boolean.h>

std::string const viua::types::Boolean::type_name = "Boolean";

auto viua::types::Boolean::type() const -> std::string
{
    return type_name;
}

auto viua::types::Boolean::str() const -> std::string
{
    return (b ? "true" : "false");
}

auto viua::types::Boolean::boolean() const -> bool
{
    return b;
}

auto viua::types::Boolean::value() -> bool&
{
    return b;
}

auto viua::types::Boolean::copy() const -> std::unique_ptr<viua::types::Value>
{
    return std::make_unique<Boolean>(b);
}

viua::types::Boolean::Boolean(bool const v) : b(v)
{}

auto viua::types::Boolean::make(bool const v) -> std::unique_ptr<Boolean>
{
    return std::make_unique<Boolean>(v);
}
auto viua::types::Boolean::make_true() -> std::unique_ptr<Boolean>
{
    return std::make_unique<Boolean>(true);
}
auto viua::types::Boolean::make_false() -> std::unique_ptr<Boolean>
{
    return std::make_unique<Boolean>(false);
}
