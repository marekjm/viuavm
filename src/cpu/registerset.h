#ifndef WUDOO_REGISTERSET_H
#define WUDOO_REGISTERSET_H

#pragma once


typedef unsigned char mask_t;

enum REGISTER_MASKS: mask_t {
    REFERENCE       = (1 << 0),
    COPY_ON_WRITE   = (1 << 1),
    KEEP            = (1 << 2),
};


class RegisterSet {
    unsigned registerset_size;
    Object** registers;
    mask_t*  masks;

    public:
        // basic access to registers
        Object* set(unsigned, Object*);
        Object* get(unsigned);
        Object* at(unsigned);

        // mask inspection and manipulation
        void enable(unsigned, mask_t);
        void disable(unsigned, mask_t);
        void clear(unsigned);
        bool isenabled(unsigned, mask_t);

        inline unsigned size() { return registerset_size; }

        RegisterSet(unsigned sz): registerset_size(sz), registers(0), masks(0) {
            /** Create register set with specified size.
             */
            registers = new Object*[sz];
            masks = new mask_t[sz];
            for (unsigned i = 0; i < sz; ++i) {
                registers[i] = 0;
                masks[i] = 0;
            }
        }
        ~RegisterSet() {
            /** Proper destructor for register sets.
             */
            for (unsigned i = 0; i < registerset_size; ++i) {
                // do not delete if register is empty
                if (registers[i] == 0) {
                    continue;
                }
                // do not delete if register is a reference or should be kept in memory even
                // after going out of scope
                if (isenabled(i, (KEEP | REFERENCE))) {
                    continue;
                }
                delete[] registers[i];
            }
            delete[] registers;
            delete[] masks;
        }
};


#endif
