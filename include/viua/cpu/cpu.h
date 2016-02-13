#ifndef VIUA_CPU_H
#define VIUA_CPU_H

#pragma once

#include <dlfcn.h>
#include <cstdint>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <algorithm>
#include <stdexcept>
#include <mutex>
#include <viua/thread.h>


class CPU {
    friend Thread;
#ifdef AS_DEBUG_HEADER
    public:
#endif
    /*  Bytecode pointer is a pointer to program's code.
     *  Size and executable offset are metadata exported from bytecode dump.
     */
    byte* bytecode;
    uint64_t bytecode_size;
    uint64_t executable_offset;

    // vector of all threads machine is executing
    std::vector<Thread*> threads;
    std::queue<Type*> watchdog_message_buffer;
    Thread* watchdog_thread;
    decltype(threads)::size_type current_thread_index;
    std::mutex threads_mtx;

    // Global register set
    RegisterSet* regset;

    // Temporary register
    Type* tmp;

    // Map of the typesystem currently existing inside the VM.
    std::map<std::string, Prototype*> typesystem;

    /*  Function and block names mapped to bytecode addresses.
     */
    byte* jump_base;
    std::map<std::string, uint64_t> function_addresses;
    std::map<std::string, uint64_t> block_addresses;

    std::map<std::string, std::pair<std::string, byte*>> linked_functions;
    std::map<std::string, std::pair<std::string, byte*>> linked_blocks;
    std::map<std::string, std::pair<unsigned, byte*> > linked_modules;

    /*  Slot for thrown objects (typically exceptions).
     *  Can be set by user code and the CPU.
     */
    Type* thrown;
    Type* caught;

    /*  Variables set after CPU executed bytecode.
     *  They describe exit conditions of the bytecode that just stopped running.
     */
    int return_code;                // always set
    std::string return_exception;   // set if CPU stopped because of an exception
    std::string return_message;     // message set by exception

    uint64_t instruction_counter;

    /*  This is the interface between programs compiled to VM bytecode and
     *  extension libraries written in C++.
     */
    std::map<std::string, ExternalFunction*> foreign_functions;

    /** This is the mapping Viua uses to dispatch methods on pure-C++ classes.
     */
    std::map<std::string, ForeignMethod> foreign_methods;

    // Final exception for stacktracing
    Type* terminating_exception;

    /*  Methods dealing with dynamic library loading.
     */
    std::vector<void*> cxx_dynamic_lib_handles;
    void loadNativeLibrary(const std::string&);
    void loadForeignLibrary(const std::string&);

    /*  Methods dealing with typesystem related tasks.
     */
    std::vector<std::string> inheritanceChainOf(const std::string&);

    public:
        // debug and error reporting flags
        bool debug, errors;

        std::vector<std::string> commandline_arguments;

        /*  Public API of the CPU provides basic actions:
         *
         *      * load bytecode,
         *      * set its size,
         *      * tell the CPU where to start execution,
         *      * kick the CPU so it starts running,
         */
        CPU& load(byte*);
        CPU& bytes(uint64_t);
        CPU& preload();

        CPU& mapfunction(const std::string&, uint64_t);
        CPU& mapblock(const std::string&, uint64_t);

        CPU& registerExternalFunction(const std::string&, ExternalFunction*);
        CPU& removeExternalFunction(std::string);

        /// These two methods are used to inject pure-C++ classes into machine's typesystem.
        CPU& registerForeignPrototype(const std::string&, Prototype*);
        CPU& registerForeignMethod(const std::string&, ForeignMethod);

        CPU& iframe(Frame* frm = nullptr, unsigned r = DEFAULT_REGISTER_SIZE);

        Thread* spawn(Frame*, Thread* parent_thread = nullptr);
        Thread* spawnWatchdog(Frame*);

        byte* tick(decltype(threads)::size_type thread_index = 0);
        bool burst();

        int run();
        inline decltype(instruction_counter) counter() {
            return threads[current_thread_index]->counter();
        }

        inline std::tuple<int, std::string, std::string> exitcondition() {
            return std::tuple<int, std::string, std::string>(return_code, return_exception, return_message);
        }
        inline std::vector<Frame*> trace() { return threads[current_thread_index]->trace(); }
        inline byte* executionAt() { return threads[current_thread_index]->executionAt(); }

        inline bool terminated() { return (terminating_exception != nullptr); }
        inline Type* terminatedBy() { return terminating_exception; }

        CPU():
            bytecode(nullptr), bytecode_size(0), executable_offset(0),
            watchdog_thread(nullptr),
            current_thread_index(0),
            regset(nullptr),
            tmp(nullptr),
            jump_base(nullptr),
            thrown(nullptr), caught(nullptr),
            return_code(0), return_exception(""), return_message(""),
            instruction_counter(0),
            terminating_exception(nullptr),
            debug(false), errors(false)
        {}

        ~CPU() {
            /*  Destructor frees memory at bytecode pointer so make sure you passed a copy of the bytecode to the constructor
             *  if you want to keep it around after the CPU is finished.
             */
            if (bytecode) { delete[] bytecode; }

            std::map<std::string, std::pair<unsigned, byte*> >::iterator lm = linked_modules.begin();
            while (lm != linked_modules.end()) {
                std::string lkey = lm->first;
                byte *ptr = lm->second.second;

                ++lm;

                linked_modules.erase(lkey);
                delete[] ptr;
            }

            std::map<std::string, Prototype*>::iterator pr = typesystem.begin();
            while (pr != typesystem.end()) {
                std::string proto_name = pr->first;
                Prototype* proto_ptr = pr->second;

                ++pr;

                typesystem.erase(proto_name);
                delete proto_ptr;
            }

            for (unsigned i = 0; i < cxx_dynamic_lib_handles.size(); ++i) {
                dlclose(cxx_dynamic_lib_handles[i]);
            }
        }
};

#endif
