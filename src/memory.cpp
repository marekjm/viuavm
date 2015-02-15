#include <iostream>
#include <algorithm>
#include <vector>
#include "memory.h"
using namespace std;


// PUBLIC METHODS

unsigned long Memory::request(unsigned sz) {
    /** Request memory block of given size.
     */
    cout << "  * Memory::request()" << endl;
    vector<unsigned long> available;
    try {
        available = allocated[sz];
    } catch (const char*& e) {
        // just prevent throwing the exception
        allocated[sz] = vector<unsigned long>();
    }

    unsigned long block = 0;
    for (auto blk : available) {
        if (not taken[blk]) {
            block = blk;
            break;
        }
    }

    if (block == 0) {
        // no available block has been found
        // so a new one must be allocated
        block = (unsigned long)(new char[sz]);
        allocated[sz].push_back(block);
        sizes[block] = sz;
        pointers.push_back(block);
    }

    taken[block] = true;

    return block;
}

void Memory::release(unsigned long block) {
    /** Release memeory block at given address.
     */
    cout << "  * Memory::release()" << endl;
    unsigned sz = sizes[block];
    zero(block, sz);
    taken[block] = false;
}

unsigned Memory::collect() {
    /** Collect and free unused blocks of memory.
     */
    cout << "  * Memory::collect()" << endl;
    unsigned freed = 0;
    for (auto block : pointers) {
        if (not taken[block]) {
            delete[] (char**)block;
            vector<unsigned long> allocated_of_size = allocated[sizes[block]];
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

void Memory::zero(unsigned long block, unsigned size) {
    /** Clear (fill with zeros) block of memory with given size at
     *  given position.
     */
    for (unsigned i = 0; i < size; ++i) { *((char*)block) = 0; }
}
