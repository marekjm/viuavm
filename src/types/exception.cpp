/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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
using namespace std;

const string viua::types::Exception::type_name = "Exception";


string viua::types::Exception::what() const {
    /** Stay compatible with standatd exceptions and
     *  provide what() method.
     */
    return cause;
}

string viua::types::Exception::etype() const {
    /** Returns exception type.
     *
     *  Basic type is 'viua::types::Exception' and is returned by the type() method.
     *  This method returns detailed type of the exception.
     */
    return detailed_type;
}

string viua::types::Exception::type() const { return "Exception"; }
string viua::types::Exception::str() const { return cause; }
string viua::types::Exception::repr() const { return (etype() + ": " + str::enquote(cause)); }
bool viua::types::Exception::boolean() const { return true; }

unique_ptr<viua::types::Value> viua::types::Exception::copy() const {
    return unique_ptr<viua::types::Value>{new Exception(cause)};
}

viua::types::Exception::Exception(string s) : cause(s), detailed_type("Exception") {}
viua::types::Exception::Exception(string ts, string cs) : cause(cs), detailed_type(ts) {}
