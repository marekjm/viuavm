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
byte* CPU::excall(byte* addr) {
    /** Run excall instruction.
     */
    bool return_register_ref = *(bool*)addr;
    pointer::inc<bool, byte>(addr);

    int return_register_index = *(int*)addr;
    pointer::inc<int, byte>(addr);

    string call_name = string(addr);
    addr += (call_name.size()+1);

    // save return address for frame
    byte* return_address = addr;

    if (frame_new == 0) {
        throw new Exception("external function call without a frame: use `frame 0' in source code if the function takes no parameters");
    }
    // set function name and return address
    frame_new->function_name = call_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = return_register_ref;
    frame_new->place_return_value_in = return_register_index;

    Frame* frame = frame_new;

    pushFrame();

    if (external_functions.count(call_name) == 0) {
        throw new Exception("call to unregistered external function: " + call_name);
    }

    /* FIXME: second parameter should be a pointer to static registers or
     *        0 if function does not have static registers registered
     * FIXME: should external functions always have static registers allocated?
     */
    ExternalFunction* callback = external_functions.at(call_name);
    (*callback)(frame, 0, regset);

    // FIXME: woohoo! segfault!
    Type* returned = 0;
    bool returned_is_reference = false;
    int return_value_register = frames.back()->place_return_value_in;
    bool resolve_return_value_register = frames.back()->resolve_return_value_register;
    if (return_value_register != 0) {
        // we check in 0. register because it's reserved for return values
        if (uregset->at(0) == 0) {
            throw new Exception("return value requested by frame but external function did not set return register");
        }
        if (uregset->isflagged(0, REFERENCE)) {
            returned = uregset->get(0);
            returned_is_reference = true;
        } else {
            returned = uregset->get(0)->copy();
        }
    }

    dropFrame();

    // place return value
    if (returned and frames.size() > 0) {
        if (resolve_return_value_register) {
            return_value_register = static_cast<Integer*>(fetch(return_value_register))->value();
        }
        place(return_value_register, returned);
        if (returned_is_reference) {
            uregset->flag(return_value_register, REFERENCE);
        }
    }

    return return_address;
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
