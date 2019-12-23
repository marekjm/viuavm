/*
 *  Copyright (C) 2018 Marek Marecki
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

#include <sstream>
#include <viua/assembler/frontend/static_analyser.h>

namespace viua { namespace assembler { namespace frontend {
namespace static_analyser {
auto Register::operator<(Register const& that) const -> bool
{
    if (register_set < that.register_set) {
        return true;
    }
    if (register_set == that.register_set and index < that.index) {
        return true;
    }
    return false;
}
auto Register::operator==(Register const& that) const -> bool
{
    return (register_set == that.register_set) and (index == that.index);
}

Register::Register(viua::assembler::frontend::parser::Register_index const& ri)
        : index(ri.index), register_set(ri.rss)
{}

auto to_string(Register const& r) -> std::string
{
    std::ostringstream oss;

    oss << r.index << ' ';
    oss << ::viua::assembler::frontend::static_analyser::checkers::to_string(
        r.register_set);
    oss << " {";
    oss << ::viua::assembler::frontend::static_analyser::checkers::to_string(
        r.value_type);
    oss << "}";

    return oss.str();
}
}}}}  // namespace viua::assembler::frontend::static_analyser
