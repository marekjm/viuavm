/*
 *  Copyright (C) 2017 Marek Marecki
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

#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/types/text.h>
#include <viua/types/boolean.h>
#include <viua/types/exception.h>
#include <viua/process.h>
#include <viua/support/string.h>
using namespace std;


viua::internals::types::byte* viua::process::Process::optext(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    string s;
    tie(addr, s) = viua::bytecode::decoder::operands::fetch_primitive_string(addr, this);

    *target = unique_ptr<viua::types::Type>{new viua::types::Text(str::strdecode(s))};

    return addr;
}


viua::internals::types::byte* viua::process::Process::optexteq(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Type *first = nullptr, *second = nullptr;
    tie(addr, first) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    tie(addr, second) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    *target = unique_ptr<viua::types::Type>{new viua::types::Boolean(*static_cast<viua::types::Text*>(first) == *static_cast<viua::types::Text*>(second))};

    return addr;
}


viua::internals::types::byte* viua::process::Process::optextat(viua::internals::types::byte*) {
    throw new viua::types::Exception("instruction not implemented");
}


viua::internals::types::byte* viua::process::Process::optextsub(viua::internals::types::byte*) {
    throw new viua::types::Exception("instruction not implemented");
}


viua::internals::types::byte* viua::process::Process::optextlength(viua::internals::types::byte*) {
    throw new viua::types::Exception("instruction not implemented");
}


viua::internals::types::byte* viua::process::Process::optextcommonprefix(viua::internals::types::byte*) {
    throw new viua::types::Exception("instruction not implemented");
}


viua::internals::types::byte* viua::process::Process::optextcommonsuffix(viua::internals::types::byte*) {
    throw new viua::types::Exception("instruction not implemented");
}


viua::internals::types::byte* viua::process::Process::optextview(viua::internals::types::byte*) {
    throw new viua::types::Exception("instruction not implemented");
}


viua::internals::types::byte* viua::process::Process::optextconcat(viua::internals::types::byte*) {
    throw new viua::types::Exception("instruction not implemented");
}
