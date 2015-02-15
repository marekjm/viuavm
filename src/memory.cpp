#include <iostream>
#include <algorithm>
#include <vector>
#include "memory.h"
using namespace std;


// PUBLIC METHODS

void* Memory::request(unsigned sz) {
    /** Request memory block of given size.
     */
    vector<void*> available;
    try {
        available = allocated[sz];
    } catch (const char*& e) {
        // just prevent throwing the exception
        allocated[sz] = vector<void*>();
    }

    void* block = 0;
    for (auto blk : available) {
        if (not taken[blk]) {
            block = blk;
            break;
        }
    }

    if (block == 0) {
        // no available block has been found
        // so a new one must be allocated
        block = (void*)(new char[sz]);
        allocated[sz].push_back(block);
        sizes[block] = sz;
        pointers.push_back(block);
    }

    taken[block] = true;

    return block;
}

void Memory::release(void* block) {
    /** Release memeory block at given address.
     */
    unsigned sz = sizes[block];
    zero(block, sz);
    taken[block] = false;
}

unsigned Memory::collect() {
    /** Collect and free unused blocks of memory.
     */
    unsigned freed = 0;
    for (auto block : pointers) {
        if (not taken[block]) {
            delete[] (char**)block;
            freed = sizes[block];
            vector<void*> allocated_of_size = allocated[sizes[block]];
            allocated_of_size.erase(find(allocated_of_size.begin(), allocated_of_size.end(), block));
            sizes.erase(block);
            taken.erase(block);
            pointers.erase(find(pointers.begin(), pointers.end(), block));
        }
    }
    return freed;
}

unsigned Memory::used() {
    /** Report how much memory is currently used.
     */
    unsigned total = 0;
    for (auto block : pointers) { total += sizes[block]; }
    return total;
}


// PRIVATE METHODS

void Memory::zero(void* block, unsigned size) {
    /** Clear (fill with zeros) block of memory with given size at
     *  given position.
     */
    for (unsigned i = 0; i < size; ++i) { *((char*)block) = 0; }
}
