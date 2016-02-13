#include <dlfcn.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <functional>
#include <regex>
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
#include <viua/support/pointer.h>
#include <viua/support/string.h>
#include <viua/support/env.h>
#include <viua/loader.h>
#include <viua/include/module.h>
#include <viua/cpu/cpu.h>
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
    jump_base = bytecode;
    return (*this);
}

CPU& CPU::bytes(uint64_t sz) {
    /*  Set bytecode size, so the CPU can stop execution even if it doesn't reach HALT instruction but reaches
     *  bytecode address out of bounds.
     */
    bytecode_size = sz;
    return (*this);
}

CPU& CPU::preload() {
    /** This method preloads dynamic libraries specified by environment.
     */
    vector<string> preload_native = support::env::getpaths("VIUAPRELINK");
    for (unsigned i = 0; i < preload_native.size(); ++i) {
        loadNativeLibrary(preload_native[i]);
    }

    vector<string> preload_foreign = support::env::getpaths("VIUAPREIMPORT");
    for (unsigned i = 0; i < preload_foreign.size(); ++i) {
        loadForeignLibrary(preload_foreign[i]);
    }

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

CPU& CPU::registerExternalFunction(const string& name, ExternalFunction* function_ptr) {
    /** Registers external function in CPU.
     */
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

    ExternalFunctionSpec* (*exports)() = nullptr;
    if ((exports = (ExternalFunctionSpec*(*)())dlsym(handle, "exports")) == nullptr) {
        throw new Exception("failed to extract interface from module: " + module);
    }

    ExternalFunctionSpec* exported = (*exports)();

    unsigned i = 0;
    while (exported[i].name != NULL) {
        registerExternalFunction(exported[i].name, exported[i].fpointer);
        ++i;
    }

    cxx_dynamic_lib_handles.push_back(handle);
}


Thread* CPU::spawn(Frame* frm, Thread* parent_thread) {
    unique_lock<std::mutex> lck{threads_mtx};
    Thread* thrd = new Thread(frm, this, jump_base, parent_thread);
    thrd->begin();
    threads.push_back(thrd);
    return thrd;
}
Thread* CPU::spawnWatchdog(Frame* frm) {
    if (watchdog_thread != nullptr) {
        throw new Exception("watchdog thread already spawned");
    }
    unique_lock<std::mutex> lck{threads_mtx};
    cout << "CPU::spawnWatchdog()" << endl;
    Thread* thrd = new Thread(frm, this, jump_base, nullptr);
    thrd->begin();
    watchdog_thread = thrd;
    return thrd;
}

vector<string> CPU::inheritanceChainOf(const string& type_name) {
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

CPU& CPU::iframe(Frame* frm, unsigned r) {
    /** Set initial frame.
     */
    Frame *initial_frame;
    if (frm == nullptr) {
        initial_frame = new Frame(nullptr, 0, 2);
        initial_frame->function_name = "__entry";

        Vector* cmdline = new Vector();
        for (unsigned i = 0; i < commandline_arguments.size(); ++i) {
            cmdline->push(new String(commandline_arguments[i]));
        }
        initial_frame->regset->set(1, cmdline);
    } else {
        initial_frame = frm;

        /*  If a frame was supplied to us to be the initial one,
         *  set global registers to the locals of supplied frame.
         */
        delete regset;
    }

    // set global registers
    regset = new RegisterSet(r);

    Thread* t = new Thread(initial_frame, this, jump_base, nullptr);
    t->detach();
    t->priority(16);
    threads.push_back(t);

    return (*this);
}


byte* CPU::tick(decltype(threads)::size_type index) {
    byte* ip = threads[index]->tick();  // returns instruction pointer
    if (threads[index]->terminated()) {
        return nullptr;
    }
    return ip;
}

bool CPU::burst() {
    if (not threads.size()) {
        // make CPU stop if there are no threads to run
        return false;
    }

    current_thread_index = 0;
    bool ticked = false;

    decltype(threads) running_threads;
    decltype(threads) dead_threads;
    bool abort_because_of_thread_termination = false;
    for (decltype(threads)::size_type i = 0; i < threads.size(); ++i) {
        current_thread_index = i;
        auto th = threads[i];

        if (th->stopped() and th->joinable()) {
            // stopped but still joinable
            // we don't have to deal with "stopped and unjoinable" case here
            // because it is handled later (after ticking code)
            continue;
        }
        if (th->suspended()) {
            // do not execute suspended threads
            continue;
        }

        ticked = true;
        for (unsigned j = 0; j < th->priority(); ++j) {
            if (th->stopped()) {
                // remember to break if the thread stopped
                // otherwise the CPU will try to tick the thread and
                // it will crash (will try to execute instructions from 0x0 pointer)
                break;
            }
            if (th->suspended()) {
                // do not execute suspended threads
                break;
            }
            th->tick();
        }

        if (th->terminated() and not th->joinable() and th->parent() == nullptr) {
            cout << "watchdog? " << watchdog_thread << endl;
            if (watchdog_thread == nullptr) {
                cout << "[thread " << th << "]: terminated by runaway exception, aborting" << endl;
                abort_because_of_thread_termination = true;
            } else {
                cout << "[thread " << th << "]: terminated by runaway exception, passing death to watchdog" << endl;
            }
            break;
        }

        // if the thread stopped and is not joinable declare it dead and
        // schedule for removal thus shortening the vector of running threads and
        // speeding up execution
        if (th->stopped() and (not th->joinable())) {
            dead_threads.push_back(th);
        } else {
            running_threads.push_back(th);
        }
    }

    if (abort_because_of_thread_termination) {
        return false;
    }

    for (decltype(dead_threads)::size_type i = 0; i < dead_threads.size(); ++i) {
        delete dead_threads[i];
    }

    // if there were any dead threads we must rebuild the scheduled threads vector
    if (dead_threads.size()) {
        threads.erase(threads.begin(), threads.end());
        for (decltype(running_threads)::size_type i = 0; i < running_threads.size(); ++i) {
            threads.push_back(running_threads[i]);
        }
    }
    return ticked;
}

int CPU::run() {
    /*  VM CPU implementation.
     */
    if (!bytecode) {
        throw "null bytecode (maybe not loaded?)";
    }

    iframe();
    threads[0]->begin();
    while (burst());

    if (current_thread_index < threads.size() and threads[current_thread_index]->terminated()) {
        cout << "thread '0:" << hex << threads[current_thread_index] << dec << "' has terminated" << endl;
        Type* e = threads[current_thread_index]->getActiveException();
        cout << e << endl;

        return_code = 1;
        terminating_exception = e;
    }

    /*
    if (return_code == 0 and regset->at(0)) {
        // if return code if the default one and
        // return register is not unused
        // copy value of return register as return code
        try {
            return_code = static_cast<Integer*>(regset->get(0))->value();
        } catch (const Exception* e) {
            return_code = 1;
            return_exception = e->type();
            return_message = e->what();
        }
    }
    */

    // delete threads and global registers
    // otherwise we get huge memory leak
    // do not delete if execution was halted because of exception
    if (not terminated()) {
        while (threads.size()) {
            delete threads.back();
            threads.pop_back();
        }
        delete regset;
    }

    return return_code;
}
