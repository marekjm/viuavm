#include <iostream>
#include <string>
#include <sstream>
#include "../types/object.h"
#include "../types/integer.h"
#include "../types/byte.h"
#include "registerset.h"
using namespace std;


template<class T> inline void copyvalue(Object* a, Object* b) {
    /** This is a short inline, template function to copy value between two `Object` pointers of the same polymorphic type.
     *  It is used internally by CPU.
     */
    static_cast<T>(a)->value() = static_cast<T>(b)->value();
}

Object* RegisterSet::set(unsigned index, Object* object) {
    /** Put object inside register specified by give index.
     *
     *  Performs bounds checking.
     */
    if (index >= registerset_size) { throw "register access out of bounds: write"; }

    if (registers[index] != 0 and !isenabled(index, REFERENCE)) {
        // register is not empty and is not a reference - the object in it must be destroyed to avoid memory leaks
        delete registers[index];
    }
    if (isenabled(index, REFERENCE)) {
        Object* referenced = get(index);

        // it is a reference, copy value of the object
        if (referenced->type() == "Integer") { copyvalue<Integer*>(referenced, object); }
        else if (referenced->type() == "Byte") { copyvalue<Byte*>(referenced, object); }

        // and delete the newly created object to avoid leaks
        delete object;
    } else {
        //Object* old_ref_ptr = (hasrefs(index) ? registers[index] : 0);
        registers[index] = object;
        //if (old_ref_ptr) { updaterefs(old_ref_ptr, object); }
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
        oss << "read from null register: " << index;
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
     */
    registers[dst] = registers[src];    // copy pointer from first-operand register to second-operand register
    registers[src] = 0;                 // zero first-operand register
    masks[dst] = masks[src];            // copy mask
    masks[src] = 0;                     // reset mask of source register
}

void RegisterSet::swap(unsigned src, unsigned dst) {
    /** Move an object from src register to dst register.
     */
    Object* tmp = registers[src];
    registers[src] = registers[dst];
    registers[dst] = tmp;

    mask_t tmp_mask = masks[src];
    masks[src] = masks[dst];
    masks[dst] = tmp_mask;
}


void RegisterSet::enable(unsigned index, mask_t filter) {
    /** Enable masks specified by filter for register at given index.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) { throw "register access out of bounds: mask_enable"; }
    if (registers[index] == 0) {
        ostringstream oss;
        oss << "read from null register: " << index;
        throw oss.str().c_str();
    }
    masks[index] = (masks[index] | filter);
}

void RegisterSet::disable(unsigned index, mask_t filter) {
    /** Disable masks specified by filter for register at given index.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) { throw "register access out of bounds: mask_disable"; }
    if (registers[index] == 0) {
        ostringstream oss;
        oss << "read from null register: " << index;
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

bool RegisterSet::isenabled(unsigned index, mask_t filter) {
    /** Returns true if given filter is enabled for register specified by given index.
     *  Returns false otherwise.
     *
     *  Performs bounds checking.
     */
    if (index >= registerset_size) { throw "register access out of bounds: mask_isenabled"; }
    // FIXME: should throw when accessing empty register, but that breaks set()
    return (masks[index] & filter);
}
