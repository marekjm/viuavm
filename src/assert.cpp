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

#include <memory>

#include <viua/assert.h>
#include <viua/util/exceptions.h>


void viua::assertions::assert_typeof(viua::types::Value* object,
                                     std::string const& expected)
{
    /** Use this assertion when strict type checking is required.
     *
     *  Example: checking if an object is an Integer.
     */
    if (object->type() != expected) {
        throw viua::util::exceptions::make_unique_exception<Type_exception>(
            expected, object->type());
    }
}
