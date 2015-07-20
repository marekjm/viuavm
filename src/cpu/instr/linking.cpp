#include <dlfcn.h>
#include <sys/stat.h>
#include <cstdlib>
#include <iostream>
#include <viua/types/integer.h>
#include <viua/support/pointer.h>
#include <viua/include/module.h>
#include <viua/exceptions.h>
#include <viua/loader.h>
#include <viua/cpu/cpu.h>
using namespace std;


vector<string> getpaths(const string& var) {
    const char* VAR = getenv(var.c_str());
    vector<string> paths;

    if (VAR == 0) {
        // return empty vector as environment does not
        // set requested variable
        return paths;
    }

    string PATH = string(VAR);

    string path;
    unsigned i = 0;
    while (i < PATH.size()) {
        if (PATH[i] == ':') {
            if (path.size()) {
                paths.push_back(path);
                path = "";
                ++i;
            }
        }
        path += PATH[i];
        ++i;
    }
    if (path.size()) {
        paths.push_back(path);
    }

    return paths;
}

bool isfile(const string& path) {
    struct stat sf;

    // not a file if stat returned error
    if (stat(path.c_str(), &sf) == -1) return false;
    // not a file if S_ISREG() macro returned false
    if (not S_ISREG(sf.st_mode)) return false;

    // file otherwise
    return true;
}


string getmodpath(const string& module, const vector<string>& paths) {
    string path = "";
    bool found = false;

    ostringstream oss;
    for (unsigned i = 0; i < paths.size(); ++i) {
        oss.str("");
        oss << paths[i] << '/' << module << ".vlib";
        path = oss.str();
        if (path[0] == '~') {
            oss.str("");
            oss << getenv("HOME") << path.substr(1);
            path = oss.str();
        }

        if ((found = isfile(path))) break;
    }

    return (found ? path : "");
}

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


byte* CPU::eximport(byte* addr) {
    /** Run eximport instruction.
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

    string path = module;
    path = getmodpath(module, getpaths("VIUAPATH"));
    if (path.size() == 0) { path = getmodpath(module, VIUAPATH); }
    if (path.size() == 0) { path = getmodpath(module, getpaths("VIUAAFTERPATH")); }

    if (path.size()) {
        Loader loader(path);
        loader.load();

        byte* lnk_btcd = loader.getBytecode();
        linked_modules[module] = pair<unsigned, byte*>(unsigned(loader.getBytecodeSize()), lnk_btcd);

        vector<string> fn_names = loader.getFunctions();
        map<string, uint16_t> fn_addrs = loader.getFunctionAddresses();
        for (unsigned i = 0; i < fn_names.size(); ++i) {
            string fn_linkname = fn_names[i];
            linked_functions[fn_linkname] = pair<string, byte*>(module, (lnk_btcd+fn_addrs[fn_names[i]]));
        }

        vector<string> bl_names = loader.getBlocks();
        map<string, uint16_t> bl_addrs = loader.getBlockAddresses();
        for (unsigned i = 0; i < bl_names.size(); ++i) {
            string bl_linkname = bl_names[i];
            linked_blocks[bl_linkname] = pair<string, byte*>(module, (lnk_btcd+bl_addrs[bl_linkname]));
        }
    } else {
        throw new Exception("failed to link: " + module);
    }

    return addr;
}
