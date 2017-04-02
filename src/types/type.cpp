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

#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
#include <viua/types/type.h>
#include <viua/types/pointer.h>
#include <viua/types/exception.h>
using namespace std;


unique_ptr<viua::types::Pointer> viua::types::Type::pointer(const viua::process::Process *process_of_origin) {
    return unique_ptr<viua::types::Pointer>{new viua::types::Pointer(this, process_of_origin)};
}

viua::types::Type::~Type() {
    for (auto p : pointers) {
        p->invalidate(this);
    }
}
