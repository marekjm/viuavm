#ifndef WUDOO_MEMORY_H
#define WUDOO_MEMORY_H

#pragma once

#include <map>
#include <vector>

class Memory {
    // pointers to allocated memory blocks
    std::vector<void*> pointers;
    // map containing information whether a block is currently taken
    std::map<void*, bool> taken;
    // mapping of requested sizes to pointers to allocated memory blocks of such size
    std::map<unsigned, std::vector<void*> > allocated;
    std::map<void*, unsigned> sizes;

    // fill block of given size with zeros
    void zero(void*, unsigned);

    public:
    void* request(unsigned);    // request memory block
    void release(void*);        // release a memory block
    unsigned collect();         // collect and free unused blocks

    unsigned used();            // report how much memory is at use currently
};


#endif
