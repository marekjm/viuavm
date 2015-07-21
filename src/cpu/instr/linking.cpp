#include <dlfcn.h>
#include <sys/stat.h>
#include <cstdlib>
#include <iostream>
#include <viua/types/integer.h>
#include <viua/support/pointer.h>
#include <viua/exceptions.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* CPU::import(byte* addr) {
    /** Run import instruction.
     */
    string module = string(addr);
    addr += module.size();
    loadForeignLibrary(module);
    return addr;
}

byte* CPU::link(byte* addr) {
    /** Run link instruction.
     */
    string module = string(addr);
    addr += module.size();
    loadNativeLibrary(module);
    return addr;
}
