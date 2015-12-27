#include <iostream>
#include <string>
#include <sstream>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/byte.h>
#include <viua/types/exception.h>
#include <viua/types/reference.h>
#include <viua/cpu/registerset.h>
using namespace std;


Type* RegisterSet::put(unsigned index, Type* object) {
    if (index >= registerset_size) { throw new Exception("register access out of bounds: write"); }
    registers[index] = object;
    return object;
}

Type* RegisterSet::set(unsigned index, Type* object) {
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

Type* RegisterSet::get(unsigned index) {
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

Type* RegisterSet::at(unsigned index) {
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


void RegisterSet::move(unsigned src, unsigned dst) {
    /** Move an object from src register to dst register.
     *
     *  Performs bound checking.
     *  Both source and destination can be empty.
     */
    if (src >= registerset_size) { throw new Exception("register access out of bounds: move source"); }
    if (dst >= registerset_size) { throw new Exception("register access out of bounds: move destination"); }
    registers[dst] = registers[src];    // copy pointer from first-operand register to second-operand register
    registers[src] = nullptr;           // zero first-operand register
    masks[dst] = masks[src];            // copy mask
    masks[src] = 0;                     // reset mask of source register
}

void RegisterSet::swap(unsigned src, unsigned dst) {
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

void RegisterSet::empty(unsigned here) {
    /** Empty a register.
     *
     *  Performs bound checking.
     *  Does not throw if the register is empty.
     */
    if (here >= registerset_size) { throw new Exception("register access out of bounds: empty"); }
    registers[here] = nullptr;
    masks[here] = 0;
}

void RegisterSet::free(unsigned here) {
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


void RegisterSet::flag(unsigned index, mask_t filter) {
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

void RegisterSet::unflag(unsigned index, mask_t filter) {
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

void RegisterSet::clear(unsigned index) {
    /** Clear masks for given register.
     *
     *  Performs bounds checking.
     */
    if (index >= registerset_size) { throw new Exception("register access out of bounds: mask_clear"); }
    masks[index] = 0;
}

bool RegisterSet::isflagged(unsigned index, mask_t filter) {
    /** Returns true if given filter is enabled for register specified by given index.
     *  Returns false otherwise.
     *
     *  Performs bounds checking.
     */
    if (index >= registerset_size) { throw new Exception("register access out of bounds: mask_isenabled"); }
    // FIXME: should throw when accessing empty register, but that breaks set()
    return (masks[index] & filter);
}

void RegisterSet::setmask(unsigned index, mask_t mask) {
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

mask_t RegisterSet::getmask(unsigned index) {
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

        // FIXME: remove this print
        //cout << "deleting: " << registers[i]->type() << " at " << hex << registers[i] << dec << endl;
        delete registers[i];
    }
    if (registers != nullptr) { delete[] registers; }
    if (masks != nullptr) { delete[] masks; }
}
