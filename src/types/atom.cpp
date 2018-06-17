/*
 *  Copyright (C) 2017, 2018 Marek Marecki
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

#include <viua/support/string.h>
#include <viua/types/atom.h>

std::string const viua::types::Atom::type_name = "viua::types::Atom";

auto viua::types::Atom::type() const -> std::string {
    return "viua::types::Atom";
}

auto viua::types::Atom::boolean() const -> bool {
    return true;
}

auto viua::types::Atom::str() const -> std::string {
    return str::enquote(value, '\'');
}

auto viua::types::Atom::repr() const -> std::string {
    return str();
}

viua::types::Atom::operator std::string() const {
    return value;
}

auto viua::types::Atom::copy() const -> std::unique_ptr<viua::types::Value> {
    return std::make_unique<Atom>(value);
}

auto viua::types::Atom::operator==(Atom const& that) const -> bool {
    return (value == that.value);
}

viua::types::Atom::Atom(std::string s) : value(s) {}
