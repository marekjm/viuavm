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

#include <sstream>
#include <string>
#include <viua/types/type.h>
#include <viua/printutils.h>
using namespace std;

string stringifyFunctionInvocation(const Frame* frame) {
    ostringstream oss;
    oss << frame->function_name << '/' << frame->args->size();
    oss << '(';
    for (unsigned i = 0; i < frame->args->size(); ++i) {
        auto optr = frame->args->at(i);
        if (optr == nullptr) {
            oss << "<moved or void>";
        } else {
            oss << optr->repr();
        }
        if (i < (frame->args->size()-1)) {
            oss << ", ";
        }
    }
    oss << ')';
    return oss.str();
}
