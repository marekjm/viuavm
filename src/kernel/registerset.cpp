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
#include <viua/types/byte.h>
#include <viua/types/exception.h>
#include <viua/types/reference.h>
#include <viua/kernel/registerset.h>
using namespace std;


Type* RegisterSet::put(registerset_size_type index, Type* object) {
    if (index >= registerset_size) { throw new Exception("register access out of bounds: write"); }
    registers[index] = object;
    return object;
}

Type* RegisterSet::pop(registerset_size_type index) {
    /** Pop an object from the register.
     */
    Type* object = at(index);
    if (not object) {
        // FIXME: throw an exception on read from empty register
    }
    empty(index);
    return object;
}

Type* RegisterSet::set(registerset_size_type index, Type* object) {
    /** Put object inside register specified by given index.
     *
     *  Performs bounds checking.
     */
    if (index >= registerset_size) { throw new Exception("register access out of bounds: write"); }

    if (registers[index] == nullptr) {
        registers[index] = object;
    } else if (dynamic_cast<Reference*>(registers[index])) {
        static_cast<Reference*>(registers[index])->rebind(object);
    } else {
        delete registers[index];
        registers[index] = object;
    }

    return object;
}

Type* RegisterSet::get(registerset_size_type index) {
    /** Fetch object from register specified by given index.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) {
        ostringstream emsg;
        emsg << "register access out of bounds: read from " << index;
        throw new Exception(emsg.str());
    }
    Type* optr = registers[index];
    if (optr == nullptr) {
        ostringstream oss;
        oss << "(get) read from null register: " << index;
        throw new Exception(oss.str());
    }
    return optr;
}

Type* RegisterSet::at(registerset_size_type index) {
    /** Fetch object from register specified by given index.
     *
     *  Performs bounds checking.
     *  Returns 0 when accessing empty register.
     */
    if (index >= registerset_size) {
        ostringstream emsg;
        emsg << "register access out of bounds: read from " << index;
        throw new Exception(emsg.str());
    }
    return registers[index];
}


void RegisterSet::move(registerset_size_type src, registerset_size_type dst) {
    /** Move an object from src register to dst register.
     *
     *  Performs bound checking.
     *  Both source and destination can be empty.
     */
    if (src >= registerset_size) { throw new Exception("register access out of bounds: move source"); }
    if (dst >= registerset_size) { throw new Exception("register access out of bounds: move destination"); }
    set(dst, pop(src));
}

void RegisterSet::swap(registerset_size_type src, registerset_size_type dst) {
    /** Swap objects in src and dst registers.
     *
     *  Performs bound checking.
     *  Both source and destination can be empty.
     */
    if (src >= registerset_size) { throw new Exception("register access out of bounds: swap source"); }
    if (dst >= registerset_size) { throw new Exception("register access out of bounds: swap destination"); }
    Type* tmp = registers[src];
    registers[src] = registers[dst];
    registers[dst] = tmp;

    mask_t tmp_mask = masks[src];
    masks[src] = masks[dst];
    masks[dst] = tmp_mask;
}

void RegisterSet::empty(registerset_size_type here) {
    /** Empty a register.
     *
     *  Performs bound checking.
     *  Does not throw if the register is empty.
     */
    if (here >= registerset_size) { throw new Exception("register access out of bounds: empty"); }
    registers[here] = nullptr;
    masks[here] = 0;
}

void RegisterSet::free(registerset_size_type here) {
    /** Free an object inside a register.
     *
     *  Performs bound checking.
     *  Throws if the register is empty.
     */
    if (here >= registerset_size) { throw new Exception("register access out of bounds: free"); }
    if (registers[here] == nullptr) { throw new Exception("invalid free: trying to free a null pointer"); }
    delete registers[here];
    empty(here);
}


void RegisterSet::flag(registerset_size_type index, mask_t filter) {
    /** Enable masks specified by filter for register at given index.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) { throw new Exception("register access out of bounds: mask_enable"); }
    if (registers[index] == nullptr) {
        ostringstream oss;
        oss << "(flag) flagging null register: " << index;
        throw new Exception(oss.str());
    }
    masks[index] = (masks[index] | filter);
}

void RegisterSet::unflag(registerset_size_type index, mask_t filter) {
    /** Disable masks specified by filter for register at given index.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) { throw new Exception("register access out of bounds: mask_disable"); }
    if (registers[index] == nullptr) {
        ostringstream oss;
        oss << "(unflag) unflagging null register: " << index;
        throw new Exception(oss.str());
    }
    masks[index] = (masks[index] ^ filter);
}

void RegisterSet::clear(registerset_size_type index) {
    /** Clear masks for given register.
     *
     *  Performs bounds checking.
     */
    if (index >= registerset_size) { throw new Exception("register access out of bounds: mask_clear"); }
    masks[index] = 0;
}

bool RegisterSet::isflagged(registerset_size_type index, mask_t filter) {
    /** Returns true if given filter is enabled for register specified by given index.
     *  Returns false otherwise.
     *
     *  Performs bounds checking.
     */
    if (index >= registerset_size) { throw new Exception("register access out of bounds: mask_isenabled"); }
    // FIXME: should throw when accessing empty register, but that breaks set()
    return (masks[index] & filter);
}

void RegisterSet::setmask(registerset_size_type index, mask_t mask) {
    /** Set mask for a register.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) { throw new Exception("register access out of bounds: mask_disable"); }
    if (registers[index] == nullptr) {
        ostringstream oss;
        oss << "(setmask) setting mask for null register: " << index;
        throw new Exception(oss.str());
    }
    masks[index] = mask;
}

mask_t RegisterSet::getmask(registerset_size_type index) {
    /** Get mask of a register.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) { throw new Exception("register access out of bounds: mask_disable"); }
    if (registers[index] == nullptr) {
        ostringstream oss;
        oss << "(getmask) getting mask of null register: " << index;
        throw new Exception(oss.str());
    }
    return masks[index];
}


void RegisterSet::drop() {
    /** Drop register set contents.
     *
     *  This function makes the register set drop all objects it holds by
     *  emptying all its available registers.
     *  No objects will be deleted, so use this function carefully.
     */
    for (unsigned i = 0; i < size(); ++i) { empty(i); }
}


RegisterSet* RegisterSet::copy() {
    RegisterSet* rscopy = new RegisterSet(size());
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

RegisterSet::RegisterSet(registerset_size_type sz): registerset_size(sz), registers(nullptr), masks(nullptr) {
    /** Create register set with specified size.
     */
    if (sz > 0) {
        registers = new Type*[sz];
        masks = new mask_t[sz];
        for (unsigned i = 0; i < sz; ++i) {
            registers[i] = nullptr;
            masks[i] = 0;
        }
    }
}
RegisterSet::~RegisterSet() {
    /** Proper destructor for register sets.
     */
    for (unsigned i = 0; i < registerset_size; ++i) {
        // do not delete if register is empty
        if (registers[i] == nullptr) { continue; }

        // do not delete if register is a reference or should be kept in memory even
        // after going out of scope
        if (isflagged(i, (KEEP | REFERENCE | BOUND))) { continue; }

        delete registers[i];
    }
    if (registers != nullptr) { delete[] registers; }
    if (masks != nullptr) { delete[] masks; }
}
