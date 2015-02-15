#ifndef WUDOO_MEMORY_H
#define WUDOO_MEMORY_H

#pragma once

#include <map>
#include <vector>

class Memory {
    // pointers to allocated memory blocks
    std::vector<unsigned long> pointers;
    // map containing information whether a block is currently taken
    std::map<unsigned long, bool> taken;
    // mapping of requested sizes to pointers to allocated memory blocks of such size
    std::map<unsigned, std::vector<unsigned long> > allocated;
    std::map<unsigned long, unsigned> sizes;

    // fill block of given size with zeros
    void zero(unsigned long, unsigned);

    public:
    unsigned long request(unsigned);    // request memory block
    void release(unsigned long);        // release a memory block
    unsigned collect();                 // collect and free unused blocks

    unsigned used();                    // report how much memory is at use currently
};


#endif
