/*
 *  Copyright (C) 2016 Marek Marecki
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
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/maps.h>
#include <viua/types/integer.h>
#include <viua/types/exception.h>
#include <viua/types/reference.h>
#include <viua/types/process.h>
#include <viua/process.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
using namespace std;


PID::PID(const Process* p): associated_process(p) {
}
bool PID::operator==(const PID& that) const {
    return (associated_process == that.associated_process);
}
bool PID::operator==(const Process* that) const {
    return (associated_process == that);
}
bool PID::operator<(const PID& that) const {
    // PIDs can't really have a less-than relation
    // they are either equal or not, and that's it
    // less-than relation is implemented only so that PID objects may be used as
    // keys in std::map<>
    return (reinterpret_cast<uint64_t>(associated_process) < reinterpret_cast<uint64_t>(that.associated_process));
}
bool PID::operator>(const PID& that) const {
    // PIDs can't really have a greater-than relation
    // they are either equal or not, and that's it
    // greater-than relation is implemented only so that PID objects may be used as
    // keys in std::map<>
    return (reinterpret_cast<uint64_t>(associated_process) > reinterpret_cast<uint64_t>(that.associated_process));
}

auto PID::get() const -> decltype(associated_process) {
    return associated_process;
}
