#include <dlfcn.h>
#include <sys/stat.h>
#include <cstdlib>
#include <iostream>
#include <viua/types/integer.h>
#include <viua/support/pointer.h>
#include <viua/exceptions.h>
#include <viua/cpu/cpu.h>
using namespace std;


extern vector<string> getpaths(const string& var);
extern bool isfile(const string& path);


void* gethandle(const string& module, const vector<string>& paths) {
    void *handle = 0;

    string path;
    ostringstream oss;

    for (unsigned i = 0; (i < paths.size()) and (handle == 0); ++i) {
        oss.str("");
        oss << paths[i] << '/' << module << ".so";
        path = oss.str();
        if (path[0] == '~') {
            oss.str("");
            oss << getenv("HOME") << path.substr(1);
            path = oss.str();
        }
        handle = dlopen(path.c_str(), RTLD_LAZY);
    }

    return handle;
}


byte* CPU::import(byte* addr) {
    /** Run import instruction.
     */
    string module = string(addr);
    addr += module.size();

    void* handle = 0;
    handle = gethandle(module, getpaths("VIUAPATH"));
    if (handle == 0) { handle = gethandle(module, VIUAPATH); }
    if (handle == 0) { handle = gethandle(module, getpaths("VIUAAFTERPATH")); }

    if (handle == 0) {
        throw new Exception("LinkException", ("failed to link library: " + module));
    }

    ExternalFunctionSpec* (*exports)() = 0;
    if ((exports = (ExternalFunctionSpec*(*)())dlsym(handle, "exports")) == 0) {
        throw new Exception("failed to extract interface from module: " + module);
    }

    ExternalFunctionSpec* exported = (*exports)();

    unsigned i = 0;
    while (exported[i].name != NULL) {
        registerExternalFunction(exported[i].name, exported[i].fpointer);
        ++i;
    }

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
