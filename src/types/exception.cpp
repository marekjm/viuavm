/*
 *  Copyright (C) 2015-2017, 2020 Marek Marecki
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

#include <string>
#include <viua/types/exception.h>

std::string const viua::types::Exception::type_name = "Exception";

auto viua::types::Exception::what() const -> std::string
{
    if (value) {
        return value->str();
    }
    return description;
}

std::string viua::types::Exception::type() const
{
    return tag;
}
std::string viua::types::Exception::str() const
{
    return what();
}
std::string viua::types::Exception::repr() const
{
    return (type() + ": " + str::enquote(description));
}
bool viua::types::Exception::boolean() const { return true; }

std::unique_ptr<viua::types::Value> viua::types::Exception::copy() const
{
    throw std::make_unique<Exception>("not copyable");
}

auto viua::types::Exception::add_throw_point(Throw_point const tp)
    -> void
{
    throw_points.push_back(tp);
}

viua::types::Exception::Exception(Tag t)
    : tag{std::move(t.tag)}
{}
viua::types::Exception::Exception(std::string c)
    : description{std::move(c)}
{}
viua::types::Exception::Exception(std::unique_ptr<Value> v)
    : tag{v->type()}
    , value{std::move(v)}
{}
viua::types::Exception::Exception(Tag t, std::string c)
    : tag{std::move(t.tag)}
    , description{std::move(c)}
{}
viua::types::Exception::Exception(Tag t, std::unique_ptr<Value> v)
    : tag{std::move(t.tag)}
    , value{std::move(v)}
{}
viua::types::Exception::Exception(std::vector<Throw_point> tp, Tag t)
    : tag{std::move(t.tag)}
    , throw_points{std::move(tp)}
{}
