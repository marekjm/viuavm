#include <iostream>
#include <string>
#include <sstream>
#include "../types/object.h"
#include "../types/integer.h"
#include "../types/byte.h"
#include "registerset.h"
using namespace std;


template<class T> inline void copyvalue(Object* dst, Object* src) {
    /** This is a short inline, template function to copy value between two `Object` pointers of the same polymorphic type.
     *  It is used internally by CPU.
     */
    static_cast<T>(dst)->value() = static_cast<T>(src)->value();
}

Object* RegisterSet::set(unsigned index, Object* object) {
    /** Put object inside register specified by give index.
     *
     *  Performs bounds checking.
     */
    if (index >= registerset_size) { throw "register access out of bounds: write"; }

    if (registers[index] != 0 and !isflagged(index, REFERENCE)) {
        // register is not empty and is not a reference - the object in it must be destroyed to avoid memory leaks
        delete registers[index];
    }
    if (isflagged(index, REFERENCE)) {
        Object* referenced = get(index);

        // it is a reference, copy value of the object
        if (referenced->type() == "Integer") { copyvalue<Integer*>(referenced, object); }
        else if (referenced->type() == "Byte") { copyvalue<Byte*>(referenced, object); }

        // and delete the newly created object to avoid leaks
        delete object;
    } else {
        registers[index] = object;
    }

    return object;
}

Object* RegisterSet::get(unsigned index) {
    /** Fetch object from register specified by given index.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) { throw "register access out of bounds: read"; }
    Object* optr = registers[index];
    if (optr == 0) {
        ostringstream oss;
        oss << "(get) read from null register: " << index;
        throw oss.str().c_str();
    }
    return optr;
}

Object* RegisterSet::at(unsigned index) {
    /** Fetch object from register specified by given index.
     *
     *  Performs bounds checking.
     *  Returns 0 when accessing empty register.
     */
    if (index >= registerset_size) { throw "register access out of bounds: read"; }
    return registers[index];
}


void RegisterSet::move(unsigned src, unsigned dst) {
    /** Move an object from src register to dst register.
     *
     *  Performs bound checking.
     *  Both source and destination can be empty.
     */
    if (src >= registerset_size) { throw "register access out of bounds: move source"; }
    if (dst >= registerset_size) { throw "register access out of bounds: move destination"; }
    registers[dst] = registers[src];    // copy pointer from first-operand register to second-operand register
    registers[src] = 0;                 // zero first-operand register
    masks[dst] = masks[src];            // copy mask
    masks[src] = 0;                     // reset mask of source register
}

void RegisterSet::swap(unsigned src, unsigned dst) {
    /** Swap objects in src and dst registers.
     *
     *  Performs bound checking.
     *  Both source and destination can be empty.
     */
    if (src >= registerset_size) { throw "register access out of bounds: swap source"; }
    if (dst >= registerset_size) { throw "register access out of bounds: swap destination"; }
    Object* tmp = registers[src];
    registers[src] = registers[dst];
    registers[dst] = tmp;

    mask_t tmp_mask = masks[src];
    masks[src] = masks[dst];
    masks[dst] = tmp_mask;
}

void RegisterSet::empty(unsigned here) {
    /** Empty a register.
     *
     *  Performs bound checking.
     *  Does not throw if the register is empty.
     */
    if (here >= registerset_size) { throw "register access out of bounds: empty"; }
    registers[here] = 0;
    masks[here] = 0;
}

void RegisterSet::free(unsigned here) {
    /** Free an object inside a register.
     *
     *  Performs bound checking.
     *  Throws if the register is empty.
     */
    if (here >= registerset_size) { throw "register access out of bounds: free"; }
    if (registers[here] == 0) { throw "invalid free: trying to free a null pointer"; }
    delete registers[here];
    empty(here);
}


void RegisterSet::flag(unsigned index, mask_t filter) {
    /** Enable masks specified by filter for register at given index.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) { throw "register access out of bounds: mask_enable"; }
    if (registers[index] == 0) {
        ostringstream oss;
        oss << "(flag) flagging null register: " << index;
        throw oss.str().c_str();
    }
    masks[index] = (masks[index] | filter);
}

void RegisterSet::unflag(unsigned index, mask_t filter) {
    /** Disable masks specified by filter for register at given index.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) { throw "register access out of bounds: mask_disable"; }
    if (registers[index] == 0) {
        ostringstream oss;
        oss << "(unflag) unflagging null register: " << index;
        throw oss.str().c_str();
    }
    masks[index] = (masks[index] ^ filter);
}

void RegisterSet::clear(unsigned index) {
    /** Clear masks for given register.
     *
     *  Performs bounds checking.
     */
    if (index >= registerset_size) { throw "register access out of bounds: mask_clear"; }
    masks[index] = 0;
}

bool RegisterSet::isflagged(unsigned index, mask_t filter) {
    /** Returns true if given filter is enabled for register specified by given index.
     *  Returns false otherwise.
     *
     *  Performs bounds checking.
     */
    if (index >= registerset_size) { throw "register access out of bounds: mask_isenabled"; }
    // FIXME: should throw when accessing empty register, but that breaks set()
    return (masks[index] & filter);
}

void RegisterSet::setmask(unsigned index, mask_t mask) {
    /** Set mask for a register.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) { throw "register access out of bounds: mask_disable"; }
    if (registers[index] == 0) {
        ostringstream oss;
        oss << "(setmask) setting mask for null register: " << index;
        throw oss.str().c_str();
    }
    masks[index] = mask;
}

mask_t RegisterSet::getmask(unsigned index) {
    /** Get mask of a register.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) { throw "register access out of bounds: mask_disable"; }
    if (registers[index] == 0) {
        ostringstream oss;
        oss << "(getmask) getting mask of null register: " << index;
        throw oss.str().c_str();
    }
    return masks[index];
}


RegisterSet* RegisterSet::copy() {
    RegisterSet* rscopy = new RegisterSet(size());
    for (unsigned i = 0; i < size(); ++i) {
        if (at(i) == 0) { continue; }

        if (isflagged(i, (REFERENCE | BOUND))) {
            rscopy->set(i, at(i));
        } else {
            rscopy->set(i, at(i)->copy());
        }
        rscopy->setmask(i, getmask(i));
    }
    return rscopy;
}

RegisterSet::RegisterSet(unsigned sz): registerset_size(sz), registers(0), masks(0) {
    /** Create register set with specified size.
     */
    if (sz > 0) {
        registers = new Object*[sz];
        masks = new mask_t[sz];
        for (unsigned i = 0; i < sz; ++i) {
            registers[i] = 0;
            masks[i] = 0;
        }
    }
}
RegisterSet::~RegisterSet() {
    /** Proper destructor for register sets.
     */
    for (unsigned i = 0; i < registerset_size; ++i) {
        // do not delete if register is empty
        if (registers[i] == 0) { continue; }

        // do not delete if register is a reference or should be kept in memory even
        // after going out of scope
        if (isflagged(i, (KEEP | REFERENCE | BOUND))) { continue; }

        // FIXME: remove this print
        //cout << "deleting: " << registers[i]->type() << " at " << hex << registers[i] << dec << endl;
        delete registers[i];
    }
    if (registers != 0) { delete[] registers; }
    if (masks != 0) { delete[] masks; }
}
