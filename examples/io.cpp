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
#include <fstream>
#include <sstream>
#include <string>
#include <viua/types/string.h>
#include <viua/types/vector.h>
#include <viua/include/module.h>
using namespace std;

void io_getline(Frame* frame, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*) {
    string s;
    getline(cin, s);
    frame->regset->set(0, new String(s));
}

void io_read(Frame* frame, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*) {
    ifstream in(dynamic_cast<String*>(frame->args->get(0))->value());
    ostringstream oss;
    string line;

    while (getline(in, line)) { oss << line << '\n'; }
    frame->regset->set(0, new String(oss.str()));
}

void io_write(Frame* frame, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*) {
    ofstream out(dynamic_cast<String*>(frame->args->get(0))->value());
    out << dynamic_cast<String*>(frame->args->get(1))->value();
    out.close();
}


const char* function_names[] = {
    "io::getline",
    "io::read",
    "io::write",
    NULL,
};
const ForeignFunction* function_pointers[] = {
    &io_getline,
    &io_read,
    &io_write,
    NULL,
};


extern "C" const char** exports_names() {
    return function_names;
}
extern "C" const ForeignFunction** exports_pointers() {
    return function_pointers;
}
