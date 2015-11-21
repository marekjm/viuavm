#include <dlfcn.h>
#include <sys/stat.h>
#include <cstdlib>
#include <iostream>
#include <viua/types/integer.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::import(byte* addr) {
    /** Run import instruction.
     */
    string module = string(addr);
    addr += module.size();
    cpu->loadForeignLibrary(module);
    return addr;
}

byte* Thread::link(byte* addr) {
    /** Run link instruction.
     */
    string module = string(addr);
    addr += module.size();
    cpu->loadNativeLibrary(module);
    return addr;
}
