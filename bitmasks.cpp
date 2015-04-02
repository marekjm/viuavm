#include <iostream>
using namespace std;

/*
 * 0  or 0 = 0
 * 0  or 1 = 1
 * 1  or 1 = 1
 * 0 and 0 = 0
 * 0 and 1 = 0
 * 1 and 1 = 1
 * 0 xor 0 = 1
 * 0 xor 1 = 0
 * 1 xor 1 = 0
 *
 *
 * 0000 & 0001 = 0000
 * 1110
 *
 */


typedef unsigned char mask_t;


enum REGISTER_MASKS: mask_t {
    REFERENCE     = (1 << 0),
    COPY_ON_WRITE = (1 << 1),
    PROTECTED     = (1 << 2),
};


void bitshow(mask_t u) {
    unsigned bits = (sizeof(mask_t)*8);
    mask_t mask = (1 << (bits-1));
    for (mask_t i = 0; i < bits; ++i) {
        cout << ((u & mask) == 0 ? '0' : '1');
        mask = (mask >> 1);
    }
    cout << endl;
}


bool enabled(mask_t mask, mask_t filter) {
    return (mask & filter);
}

mask_t enable(mask_t mask, mask_t filter) {
    return (mask | filter);
}

mask_t disable(mask_t mask, mask_t filter) {
    return (mask ^ filter);
}

mask_t clear(mask_t mask) {
    return 0;
}


int main() {
    mask_t b = 0;


    b = enable(b, REFERENCE);
    b = enable(b, PROTECTED);
    bitshow(b);
    cout << "is reference: " << enabled(b, REFERENCE) << endl;
    cout << "is copy-on-write: " << enabled(b, COPY_ON_WRITE) << endl;
    cout << "is protected: " << enabled(b, PROTECTED) << endl;
    cout << "is protected reference: " << enabled(b, (PROTECTED | REFERENCE)) << endl;


    b = disable(b, REFERENCE);
    bitshow(b);
    cout << "is reference: " << enabled(b, REFERENCE) << endl;
    cout << "is copy-on-write: " << enabled(b, COPY_ON_WRITE) << endl;
    cout << "is protected: " << enabled(b, PROTECTED) << endl;


    b = clear(b);
    bitshow(b);
    cout << "is reference: " << enabled(b, REFERENCE) << endl;
    cout << "is copy-on-write: " << enabled(b, COPY_ON_WRITE) << endl;
    cout << "is protected: " << enabled(b, PROTECTED) << endl;


    return 0;
}
