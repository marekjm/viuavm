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

#include <dlfcn.h>
#include <sys/stat.h>
#include <cstdlib>
#include <iostream>
#include <viua/types/integer.h>
#include <viua/operand.h>
#include <viua/kernel/opex.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
using namespace std;


byte* Process::opimport(byte* addr) {
    /** Run import instruction.
     */
    string module = viua::operand::extractString(addr);
    scheduler->loadForeignLibrary(module);
    return addr;
}

byte* Process::oplink(byte* addr) {
    /** Run link instruction.
     */
    string module = viua::operand::extractString(addr);
    scheduler->loadNativeLibrary(module);
    return addr;
}
