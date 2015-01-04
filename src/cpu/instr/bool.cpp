#include <iostream>
#include <vector>
#include "../../bytecode.h"
#include "../../types/object.h"
#include "../../types/integer.h"
#include "../../types/boolean.h"
#include "../../types/byte.h"
#include "../../types/boolean.h"
#include "../../support/pointer.h"
#include "../cpu.h"
using namespace std;


char* CPU::lognot(char* addr) {
    /*  Run idec instruction.
     */
    bool ref = false;
    int regno;

    ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);

    regno = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "NOT" << (ref ? " @" : " ") << regno;
    }

    if (ref) {
        regno = static_cast<Integer*>(fetch(regno))->value();
    }

    if (debug) {
        if (ref) { cout << " -> " << regno; }
        cout << endl;
    }

    place(regno, new Boolean(not fetch(regno)->boolean()));

    return addr;
}

char* CPU::logand(char* addr) {
    /*  Run ieq instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "AND";
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
        cout << endl;
    }

    if (rega_ref) {
        if (debug) { cout << "resolving reference to a-operand register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }
    if (regb_ref) {
        if (debug) { cout << "resolving reference to b-operand register" << endl; }
        regb_num = static_cast<Integer*>(registers[regb_num])->value();
    }
    if (regr_ref) {
        if (debug) { cout << "resolving reference to result register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Boolean(fetch(rega_num)->boolean() and fetch(regb_num)->boolean()));

    return addr;
}

char* CPU::logor(char* addr) {
    /*  Run ieq instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "OR";
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
        cout << endl;
    }

    if (rega_ref) {
        if (debug) { cout << "resolving reference to a-operand register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }
    if (regb_ref) {
        if (debug) { cout << "resolving reference to b-operand register" << endl; }
        regb_num = static_cast<Integer*>(registers[regb_num])->value();
    }
    if (regr_ref) {
        if (debug) { cout << "resolving reference to result register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Boolean(fetch(rega_num)->boolean() or fetch(regb_num)->boolean()));

    return addr;
}
