#include <dlfcn.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <functional>
#include <regex>
#include <chrono>
#include <viua/machine.h>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/maps.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/byte.h>
#include <viua/types/string.h>
#include <viua/types/vector.h>
#include <viua/types/exception.h>
#include <viua/types/reference.h>
#include <viua/types/object.h>
#include <viua/types/function.h>
#include <viua/support/pointer.h>
#include <viua/support/string.h>
#include <viua/support/env.h>
#include <viua/loader.h>
#include <viua/include/module.h>
#include <viua/cpu/cpu.h>
#include <viua/scheduler/vps.h>
using namespace std;


CPU& CPU::load(byte* bc) {
    /*  Load bytecode into the CPU.
     *  CPU becomes owner of loaded bytecode - meaning it will consider itself responsible for proper
     *  destruction of it, so make sure you have a copy of the bytecode.
     *
     *  Any previously loaded bytecode is freed.
     *  To free bytecode without loading anything new it is possible to call .load(0).
     *
     *  :params:
     *
     *  bc:char*    - pointer to byte array containing bytecode with a program to run
     */
    if (bytecode) { delete[] bytecode; }
    bytecode = bc;
    return (*this);
}

CPU& CPU::bytes(uint64_t sz) {
    /*  Set bytecode size, so the CPU can stop execution even if it doesn't reach HALT instruction but reaches
     *  bytecode address out of bounds.
     */
    bytecode_size = sz;
    return (*this);
}

CPU& CPU::mapfunction(const string& name, uint64_t address) {
    /** Maps function name to bytecode address.
     */
    function_addresses[name] = address;
    return (*this);
}

CPU& CPU::mapblock(const string& name, uint64_t address) {
    /** Maps block name to bytecode address.
     */
    block_addresses[name] = address;
    return (*this);
}

CPU& CPU::registerExternalFunction(const string& name, ForeignFunction* function_ptr) {
    /** Registers external function in CPU.
     */
    unique_lock<mutex> lock(foreign_functions_mutex);
    foreign_functions[name] = function_ptr;
    return (*this);
}

CPU& CPU::registerForeignPrototype(const string& name, Prototype* proto) {
    /** Registers foreign prototype in CPU.
     */
    typesystem[name] = proto;
    return (*this);
}

CPU& CPU::registerForeignMethod(const string& name, ForeignMethod method) {
    /** Registers foreign prototype in CPU.
     */
    foreign_methods[name] = method;
    return (*this);
}


void CPU::loadNativeLibrary(const string& module) {
    regex double_colon("::");
    ostringstream oss;
    oss << regex_replace(module, double_colon, "/");
    string try_path = oss.str();
    string path = support::env::viua::getmodpath(try_path, "vlib", support::env::getpaths("VIUAPATH"));
    if (path.size() == 0) { path = support::env::viua::getmodpath(try_path, "vlib", VIUAPATH); }
    if (path.size() == 0) { path = support::env::viua::getmodpath(try_path, "vlib", support::env::getpaths("VIUAAFTERPATH")); }

    if (path.size()) {
        Loader loader(path);
        loader.load();

        byte* lnk_btcd = loader.getBytecode();
        linked_modules[module] = pair<unsigned, byte*>(static_cast<unsigned>(loader.getBytecodeSize()), lnk_btcd);

        vector<string> fn_names = loader.getFunctions();
        map<string, uint64_t> fn_addrs = loader.getFunctionAddresses();
        for (unsigned i = 0; i < fn_names.size(); ++i) {
            string fn_linkname = fn_names[i];
            linked_functions[fn_linkname] = pair<string, byte*>(module, (lnk_btcd+fn_addrs[fn_names[i]]));
        }

        vector<string> bl_names = loader.getBlocks();
        map<string, uint64_t> bl_addrs = loader.getBlockAddresses();
        for (unsigned i = 0; i < bl_names.size(); ++i) {
            string bl_linkname = bl_names[i];
            linked_blocks[bl_linkname] = pair<string, byte*>(module, (lnk_btcd+bl_addrs[bl_linkname]));
        }
    } else {
        throw new Exception("failed to link: " + module);
    }
}
void CPU::loadForeignLibrary(const string& module) {
    string path = "";
    path = support::env::viua::getmodpath(module, "so", support::env::getpaths("VIUAPATH"));
    if (path.size() == 0) { path = support::env::viua::getmodpath(module, "so", VIUAPATH); }
    if (path.size() == 0) { path = support::env::viua::getmodpath(module, "so", support::env::getpaths("VIUAAFTERPATH")); }

    if (path.size() == 0) {
        throw new Exception("LinkException", ("failed to link library: " + module));
    }

    void* handle = dlopen(path.c_str(), RTLD_LAZY);

    if (handle == nullptr) {
        throw new Exception("LinkException", ("failed to open handle: " + module));
    }

    ForeignFunctionSpec* (*exports)() = nullptr;
    if ((exports = reinterpret_cast<ForeignFunctionSpec*(*)()>(dlsym(handle, "exports"))) == nullptr) {
        throw new Exception("failed to extract interface from module: " + module);
    }

    ForeignFunctionSpec* exported = (*exports)();

    unsigned i = 0;
    while (exported[i].name != NULL) {
        registerExternalFunction(exported[i].name, exported[i].fpointer);
        ++i;
    }

    cxx_dynamic_lib_handles.push_back(handle);
}


Process* CPU::spawn(Frame* frm, Process* parent_process) {
    unique_lock<std::mutex> lck{processes_mtx};
    Process* thrd = new Process(frm, base_vps, parent_process);
    thrd->begin();
    processes.push_back(thrd);
    return thrd;
}
Process* CPU::spawnWatchdog(Frame* frm) {
    if (watchdog_process != nullptr) {
        throw new Exception("watchdog process already spawned");
    }
    unique_lock<std::mutex> lck{processes_mtx};
    watchdog_function = frm->function_name;
    Process* thrd = new Process(frm, base_vps, nullptr);
    thrd->begin();
    watchdog_process = thrd;
    return thrd;
}
void CPU::resurrectWatchdog() {
    auto active_exception = watchdog_process->getActiveException();
    if (active_exception) {
        cout << "watchdog process terminated by: " << active_exception->type() << ": '" << active_exception->str() << "'" << endl;
        delete active_exception;
    }

    delete watchdog_process;
    watchdog_process = nullptr;

    Frame *frm = nullptr;
    frm = new Frame(nullptr, 0, watchdog_process_register_count);
    frm->function_name = watchdog_function;

    spawnWatchdog(frm);
}

bool CPU::isClass(const string& name) const {
    return typesystem.count(name);
}

bool CPU::classAccepts(const string& klass, const string& method_name) const {
    return typesystem.at(klass)->accepts(method_name);
}

vector<string> CPU::inheritanceChainOf(const string& type_name) const {
    /** This methods returns full inheritance chain of a type.
     */
    if (typesystem.count(type_name) == 0) {
        // FIXME: better exception message
        throw new Exception("unregistered type: " + type_name);
    }
    vector<string> ichain = typesystem.at(type_name)->getAncestors();
    for (unsigned i = 0; i < ichain.size(); ++i) {
        vector<string> sub_ichain = inheritanceChainOf(ichain[i]);
        for (unsigned j = 0; j < sub_ichain.size(); ++j) {
            ichain.push_back(sub_ichain[j]);
        }
    }

    vector<string> linearised_inheritance_chain;
    unordered_set<string> pushed;

    string element;
    for (unsigned i = 0; i < ichain.size(); ++i) {
        element = ichain[i];
        if (pushed.count(element)) {
            linearised_inheritance_chain.erase(remove(linearised_inheritance_chain.begin(), linearised_inheritance_chain.end(), element), linearised_inheritance_chain.end());
        } else {
            pushed.insert(element);
        }
        linearised_inheritance_chain.push_back(element);
    }

    return ichain;
}

bool CPU::isLocalFunction(const string& name) const {
    return function_addresses.count(name);
}

bool CPU::isLinkedFunction(const string& name) const {
    return linked_functions.count(name);
}

bool CPU::isNativeFunction(const string& name) const {
    return (function_addresses.count(name) or linked_functions.count(name));
}

bool CPU::isForeignMethod(const string& name) const {
    return foreign_methods.count(name);
}

bool CPU::isForeignFunction(const string& name) const {
    return foreign_functions.count(name);
}

bool CPU::isBlock(const string& name) const {
    return (block_addresses.count(name) or linked_blocks.count(name));
}

bool CPU::isLocalBlock(const string& name) const {
    return block_addresses.count(name);
}

bool CPU::isLinkedBlock(const string& name) const {
    return linked_blocks.count(name);
}

pair<byte*, byte*> CPU::getEntryPointOfBlock(const std::string& name) const {
    byte *entry_point = nullptr;
    byte *module_base = nullptr;
    if (block_addresses.count(name)) {
        entry_point = (bytecode + block_addresses.at(name));
        module_base = bytecode;
    } else {
        auto lf = linked_blocks.at(name);
        entry_point = lf.second;
        module_base = linked_modules.at(lf.first).second;
    }
    return pair<byte*, byte*>(entry_point, module_base);
}

string CPU::resolveMethodName(const string& klass, const string& method_name) const {
    return typesystem.at(klass)->resolvesTo(method_name);
}

pair<byte*, byte*> CPU::getEntryPointOf(const std::string& name) const {
    byte *entry_point = nullptr;
    byte *module_base = nullptr;
    if (function_addresses.count(name)) {
        entry_point = (bytecode + function_addresses.at(name));
        module_base = bytecode;
    } else {
        auto lf = linked_functions.at(name);
        entry_point = lf.second;
        module_base = linked_modules.at(lf.first).second;
    }
    return pair<byte*, byte*>(entry_point, module_base);
}

void CPU::registerPrototype(Prototype *proto) {
    typesystem[proto->getTypeName()] = proto;
}

void CPU::requestForeignFunctionCall(Frame *frame, Process *requesting_process) {
    unique_lock<mutex> lock(foreign_call_queue_mutex);
    foreign_call_queue.push_back(new ForeignFunctionCallRequest(frame, requesting_process, this));

    // unlock before calling notify_one() to avoid waking the worker thread when it
    // cannot obtain the lock and
    // fetch the call request
    lock.unlock();
    foreign_call_queue_condition.notify_one();
}

void CPU::requestForeignMethodCall(const string& name, Type *object, Frame *frame, RegisterSet*, RegisterSet*, Process *p) {
    foreign_methods.at(name)(object, frame, nullptr, nullptr, p, this);
}

int CPU::exit() const {
    return return_code;
}

int CPU::run() {
    /*  VM CPU implementation.
     */
    if (!bytecode) {
        throw "null bytecode (maybe not loaded?)";
    }

    viua::scheduler::VirtualProcessScheduler vps(this, &processes);
    vps.bootstrap(commandline_arguments);

    base_vps = &vps;

    while (vps.burst());

    return_code = vps.exit();

    while (processes.size()) {
        delete processes.back();
        processes.pop_back();
    }

    return return_code;
}
