#include <dlfcn.h>
#include <sys/stat.h>
#include <cstdlib>
#include <iostream>
#include <viua/types/integer.h>
#include <viua/operand.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::opimport(byte* addr) {
    /** Run import instruction.
     */
    string module = viua::operand::extractString(addr);
    cpu->loadForeignLibrary(module);
    return addr;
}

byte* Thread::oplink(byte* addr) {
    /** Run link instruction.
     */
    string module = viua::operand::extractString(addr);
    cpu->loadNativeLibrary(module);
    return addr;
}
