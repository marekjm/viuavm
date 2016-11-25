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
#include <string>
#include <sstream>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/exception.h>
#include <viua/types/reference.h>
#include <viua/kernel/registerset.h>
using namespace std;


viua::types::Type* viua::kernel::RegisterSet::put(registerset_size_type index, viua::types::Type* object) {
    if (index >= registerset_size) { throw new viua::types::Exception("register access out of bounds: write"); }
    registers.at(index).reset(object);
    return object;
}

viua::types::Type* viua::kernel::RegisterSet::pop(registerset_size_type index) {
    /** Pop an object from the register.
     */
    viua::types::Type* object = at(index);
    if (not object) {
        // FIXME: throw an exception on read from empty register
    }
    empty(index);
    return object;
}

viua::types::Type* viua::kernel::RegisterSet::set(registerset_size_type index, viua::types::Type* object) {
    /** Put object inside register specified by given index.
     *
     *  Performs bounds checking.
     */
    if (index >= registers.size()) { throw new viua::types::Exception("register access out of bounds: write"); }

    if (dynamic_cast<viua::types::Reference*>(registers.at(index).get())) {
        static_cast<viua::types::Reference*>(registers.at(index).get())->rebind(object);
    } else {
        registers.at(index).reset(object);
    }

    return object;
}

viua::types::Type* viua::kernel::RegisterSet::get(registerset_size_type index) {
    /** Fetch object from register specified by given index.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) {
        ostringstream emsg;
        emsg << "register access out of bounds: read from " << index;
        throw new viua::types::Exception(emsg.str());
    }
    viua::types::Type* optr = registers.at(index).get();
    if (optr == nullptr) {
        ostringstream oss;
        oss << "(get) read from null register: " << index;
        throw new viua::types::Exception(oss.str());
    }
    return optr;
}

viua::types::Type* viua::kernel::RegisterSet::at(registerset_size_type index) {
    /** Fetch object from register specified by given index.
     *
     *  Performs bounds checking.
     *  Returns 0 when accessing empty register.
     */
    if (index >= registerset_size) {
        ostringstream emsg;
        emsg << "register access out of bounds: read from " << index;
        throw new viua::types::Exception(emsg.str());
    }
    return registers.at(index).get();
}


void viua::kernel::RegisterSet::move(registerset_size_type src, registerset_size_type dst) {
    /** Move an object from src register to dst register.
     *
     *  Performs bound checking.
     *  Both source and destination can be empty.
     */
    if (src >= registerset_size) { throw new viua::types::Exception("register access out of bounds: move source"); }
    if (dst >= registerset_size) { throw new viua::types::Exception("register access out of bounds: move destination"); }
    set(dst, pop(src));
}

void viua::kernel::RegisterSet::swap(registerset_size_type src, registerset_size_type dst) {
    /** Swap objects in src and dst registers.
     *
     *  Performs bound checking.
     *  Both source and destination can be empty.
     */
    if (src >= registerset_size) { throw new viua::types::Exception("register access out of bounds: swap source"); }
    if (dst >= registerset_size) { throw new viua::types::Exception("register access out of bounds: swap destination"); }
    unique_ptr<viua::types::Type> tmp = std::move(registers.at(src));
    registers[src] = std::move(registers.at(dst));
    registers[dst] = std::move(tmp);

    mask_t tmp_mask = masks.at(src);
    masks[src] = masks.at(dst);
    masks[dst] = tmp_mask;
}

void viua::kernel::RegisterSet::empty(registerset_size_type here) {
    /** Empty a register.
     *
     *  Performs bound checking.
     *  Does not throw if the register is empty.
     */
    if (here >= registerset_size) { throw new viua::types::Exception("register access out of bounds: empty"); }
    registers.at(here).release();
    masks[here] = 0;
}

void viua::kernel::RegisterSet::free(registerset_size_type here) {
    /** Free an object inside a register.
     *
     *  Performs bound checking.
     *  Throws if the register is empty.
     */
    if (here >= registerset_size) { throw new viua::types::Exception("register access out of bounds: free"); }
    if (not registers.at(here)) { throw new viua::types::Exception("invalid free: trying to free a null pointer"); }
    registers.at(here).reset(nullptr);
    empty(here);
}


void viua::kernel::RegisterSet::flag(registerset_size_type index, mask_t filter) {
    /** Enable masks specified by filter for register at given index.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) { throw new viua::types::Exception("register access out of bounds: mask_enable"); }
    if (not registers.at(index)) {
        ostringstream oss;
        oss << "(flag) flagging null register: " << index;
        throw new viua::types::Exception(oss.str());
    }
    masks[index] = (masks[index] | filter);
}

void viua::kernel::RegisterSet::unflag(registerset_size_type index, mask_t filter) {
    /** Disable masks specified by filter for register at given index.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) { throw new viua::types::Exception("register access out of bounds: mask_disable"); }
    if (not registers.at(index)) {
        ostringstream oss;
        oss << "(unflag) unflagging null register: " << index;
        throw new viua::types::Exception(oss.str());
    }
    masks[index] = (masks[index] ^ filter);
}

void viua::kernel::RegisterSet::clear(registerset_size_type index) {
    /** Clear masks for given register.
     *
     *  Performs bounds checking.
     */
    if (index >= registerset_size) { throw new viua::types::Exception("register access out of bounds: mask_clear"); }
    masks[index] = 0;
}

bool viua::kernel::RegisterSet::isflagged(registerset_size_type index, mask_t filter) {
    /** Returns true if given filter is enabled for register specified by given index.
     *  Returns false otherwise.
     *
     *  Performs bounds checking.
     */
    if (index >= registerset_size) { throw new viua::types::Exception("register access out of bounds: mask_isenabled"); }
    // FIXME: should throw when accessing empty register, but that breaks set()
    return (masks[index] & filter);
}

void viua::kernel::RegisterSet::setmask(registerset_size_type index, mask_t mask) {
    /** Set mask for a register.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) { throw new viua::types::Exception("register access out of bounds: mask_disable"); }
    if (not registers.at(index)) {
        ostringstream oss;
        oss << "(setmask) setting mask for null register: " << index;
        throw new viua::types::Exception(oss.str());
    }
    masks[index] = mask;
}

mask_t viua::kernel::RegisterSet::getmask(registerset_size_type index) {
    /** Get mask of a register.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) { throw new viua::types::Exception("register access out of bounds: mask_disable"); }
    if (not registers.at(index)) {
        ostringstream oss;
        oss << "(getmask) getting mask of null register: " << index;
        throw new viua::types::Exception(oss.str());
    }
    return masks[index];
}


void viua::kernel::RegisterSet::drop() {
    /** Drop register set contents.
     *
     *  This function makes the register set drop all objects it holds by
     *  emptying all its available registers.
     *  No objects will be deleted, so use this function carefully.
     */
    for (unsigned i = 0; i < size(); ++i) { empty(i); }
}


viua::kernel::RegisterSet* viua::kernel::RegisterSet::copy() {
    auto rscopy = new viua::kernel::RegisterSet(size());
    for (unsigned i = 0; i < size(); ++i) {
        if (at(i) == nullptr) { continue; }

        if (isflagged(i, (REFERENCE | BOUND))) {
            rscopy->set(i, at(i));
        } else {
            rscopy->set(i, at(i)->copy());
        }
        rscopy->setmask(i, getmask(i));
    }
    return rscopy;
}

viua::kernel::RegisterSet::RegisterSet(registerset_size_type sz): registerset_size(sz) {
    /** Create register set with specified size.
     */
    registers.reserve(sz);
    masks.reserve(sz);

    for (decltype(sz) i = 0; i < sz; ++i) {
        registers.emplace_back(nullptr);
        masks.emplace_back(0);
    }
}
viua::kernel::RegisterSet::~RegisterSet() {
}
