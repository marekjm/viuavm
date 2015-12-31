#ifndef VIUA_REGISTERSET_H
#define VIUA_REGISTERSET_H

#pragma once

#include "../types/type.h"

typedef unsigned char mask_t;
typedef long unsigned registerset_size_type;

enum REGISTER_MASKS: mask_t {
    REFERENCE       = (1 << 0),
    COPY_ON_WRITE   = (1 << 1),
    KEEP            = (1 << 2), // do not delete when frame is popped from stack
    BIND            = (1 << 3), // hint for closure instruction what registers to bind
    BOUND           = (1 << 4), // markes registers bound in closures
    PASSED          = (1 << 5), // marks registers containing objects passed to functions by parameter
                                // these registers *MUST NOT* be overwritten
};


class RegisterSet {
    registerset_size_type registerset_size;
    Type** registers;
    mask_t*  masks;

    public:
        // basic access to registers
        Type* put(unsigned, Type*);
        Type* pop(unsigned);
        Type* set(unsigned, Type*);
        Type* get(unsigned);
        Type* at(unsigned);

        // register modifications
        void move(unsigned, unsigned);
        void swap(unsigned, unsigned);
        void empty(unsigned);
        void free(unsigned);

        // mask inspection and manipulation
        void flag(unsigned, mask_t);
        void unflag(unsigned, mask_t);
        void clear(unsigned);
        bool isflagged(unsigned, mask_t);
        void setmask(unsigned, mask_t);
        mask_t getmask(unsigned);

        void drop();
        inline registerset_size_type size() { return registerset_size; }

        RegisterSet* copy();

        RegisterSet(registerset_size_type sz);
        ~RegisterSet();
};


#endif
