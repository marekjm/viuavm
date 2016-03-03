#ifndef VIUA_THREAD_H
#define VIUA_THREAD_H

#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/prototype.h>
#include <viua/cpu/registerset.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/tryframe.h>
#include <viua/include/module.h>


const unsigned DEFAULT_REGISTER_SIZE = 256;
const unsigned MAX_STACK_SIZE = 8192;


class HaltException : public std::runtime_error {
    public:
        HaltException(): std::runtime_error("execution halted") {}
};


class CPU;


class Thread {
#ifdef AS_DEBUG_HEADER
    public:
#endif
    CPU *cpu;
    Thread* parent_thread;
    const std::string entry_function;

    bool debug;

    // Currently used register set
    RegisterSet* regset;
    RegisterSet* uregset;
    // Temporary register
    Type* tmp;
    // Static registers
    std::map<std::string, RegisterSet*> static_registers;


    // Call stack
    byte* jump_base;
    std::vector<Frame*> frames;
    Frame* frame_new;

    // Block stack
    std::vector<TryFrame*> tryframes;
    TryFrame* try_frame_new;

    /*  Slot for thrown objects (typically exceptions).
     *  Can be set by user code and the CPU.
     */
    Type* thrown;
    Type* caught;
    bool has_unhandled_exception;

    /*  Variables set after CPU executed bytecode.
     *  They describe exit conditions of the bytecode that just stopped running.
     */
    Type* return_value; // return value of top-most frame on the stack
    int return_code;                // always set
    std::string return_exception;   // set if CPU stopped because of an exception
    std::string return_message;     // message set by exception

    // FIXME: change unsigned to uint64_t
    uint64_t instruction_counter;
    byte* instruction_pointer;

    std::queue<Type*> message_queue;

    Type* fetch(unsigned) const;
    void place(unsigned, Type*);
    void updaterefs(Type*, Type*);
    bool hasrefs(unsigned);
    void ensureStaticRegisters(std::string);

    /*  Methods dealing with stack and frame manipulation, and
     *  function calls.
     */
    Frame* requestNewFrame(int arguments_size = 0, int registers_size = 0);
    TryFrame* requestNewTryFrame();
    void pushFrame();
    void dropFrame();
    // call native (i.e. written in Viua) function
    byte* callNative(byte*, const std::string&, const bool&, const int&, const std::string&);
    // call foreign (i.e. from a C++ extension) function
    byte* callForeign(byte*, const std::string&, const bool&, const int&, const std::string&);
    // call foreign method (i.e. method of a pure-C++ class loaded into machine's typesystem)
    byte* callForeignMethod(byte*, Type*, const std::string&, const bool&, const int&, const std::string&);

    bool finished;
    bool is_joinable;
    bool is_suspended;
    unsigned thread_priority;
    std::mutex thread_mtx;

    /*  Methods implementing CPU instructions.
     */
    byte* opizero(byte*);
    byte* opistore(byte*);
    byte* opiadd(byte*);
    byte* opisub(byte*);
    byte* opimul(byte*);
    byte* opidiv(byte*);

    byte* opilt(byte*);
    byte* opilte(byte*);
    byte* opigt(byte*);
    byte* opigte(byte*);
    byte* opieq(byte*);

    byte* opiinc(byte*);
    byte* opidec(byte*);

    byte* opfstore(byte*);
    byte* opfadd(byte*);
    byte* opfsub(byte*);
    byte* opfmul(byte*);
    byte* opfdiv(byte*);

    byte* opflt(byte*);
    byte* opflte(byte*);
    byte* opfgt(byte*);
    byte* opfgte(byte*);
    byte* opfeq(byte*);

    byte* opbstore(byte*);

    byte* opitof(byte*);
    byte* opftoi(byte*);
    byte* opstoi(byte*);
    byte* opstof(byte*);

    byte* opstrstore(byte*);

    byte* opvec(byte*);
    byte* opvinsert(byte*);
    byte* opvpush(byte*);
    byte* opvpop(byte*);
    byte* opvat(byte*);
    byte* opvlen(byte*);

    byte* boolean(byte*);
    byte* opnot(byte*);
    byte* opand(byte*);
    byte* opor(byte*);

    byte* opmove(byte*);
    byte* opcopy(byte*);
    byte* opref(byte*);
    byte* opopptr(byte*);
    byte* opswap(byte*);
    byte* opdelete(byte*);
    byte* opempty(byte*);
    byte* opisnull(byte*);

    byte* opress(byte*);
    byte* optmpri(byte*);
    byte* optmpro(byte*);

    byte* print(byte*);
    byte* echo(byte*);

    byte* enclose(byte*);
    byte* openclosecopy(byte*);
    byte* openclosemove(byte*);
    byte* closure(byte*);

    byte* function(byte*);
    byte* fcall(byte*);

    byte* frame(byte*);
    byte* param(byte*);
    byte* oppamv(byte*);
    byte* paref(byte*);
    byte* arg(byte*);
    byte* argc(byte*);

    byte* call(byte*);
    byte* opthread(byte*);
    byte* opthjoin(byte*);
    byte* opthreceive(byte*);
    byte* opwatchdog(byte*);
    byte* opreturn(byte*);

    byte* jump(byte*);
    byte* branch(byte*);

    byte* vmtry(byte*);
    byte* vmcatch(byte*);
    byte* pull(byte*);
    byte* vmenter(byte*);
    byte* vmthrow(byte*);
    byte* leave(byte*);

    byte* vmclass(byte*);
    byte* prototype(byte*);
    byte* vmderive(byte*);
    byte* vmattach(byte*);
    byte* vmregister(byte*);

    byte* vmnew(byte*);
    byte* vmmsg(byte*);

    byte* opimport(byte*);
    byte* oplink(byte*);

    public:
        byte* dispatch(byte*);
        byte* xtick();
        byte* tick();

        Type* obtain(unsigned i) const {
            return fetch(i);
        }
        void put(unsigned i, Type* o) {
            place(i, o);
        }

        inline bool joinable() const { return is_joinable; }
        inline void join() {
            /** Join a thread with calling thread.
             *
             *  This function causes calling thread to be blocked until
             *  this thread has stopped.
             */
            is_joinable = false;
        }
        inline void detach() {
            /** Detach a thread.
             *
             *  This function causes the thread to become unjoinable, but
             *  allows it to run in the background.
             *
             *  Keep in mind that while detached threads cannot be joined,
             *  they can receive messages.
             *  Also, they will run even after the main/1 function has exited.
             */
            is_joinable = false;
            parent_thread = nullptr;
        }

        inline void suspend() {
            std::unique_lock<std::mutex> lck;
            is_suspended = true;
        }
        inline void wakeup() {
            std::unique_lock<std::mutex> lck;
            is_suspended = false;
        }
        inline bool suspended() {
            std::unique_lock<std::mutex> lck;
            return is_suspended;
        }

        inline Thread* parent() const { return parent_thread; };

        void pass(Type* message);

        decltype(thread_priority) priority() const { return thread_priority; }
        void priority(decltype(thread_priority) p) { thread_priority = p; }

        inline bool stopped() const { return (finished or has_unhandled_exception); }

        inline bool terminated() const { return has_unhandled_exception; }
        inline Type* getActiveException() { return thrown; }
        inline void transferActiveExceptionTo(Type*& exception_register) {
            exception_register = thrown;
            thrown = nullptr;
        }
        inline void raiseException(Type* exception) {
            thrown = exception;
        }
        inline void popFrame() {
            dropFrame();
        }
        inline Type* getReturnValue() {
            Type* tmp = return_value;
            return_value = nullptr;
            return tmp;
        }

        byte* begin();
        inline uint64_t counter() { return instruction_counter; }
        inline decltype(instruction_pointer) executionAt() { return instruction_pointer; }

        inline std::tuple<int, std::string, std::string> exitcondition() {
            return std::tuple<int, std::string, std::string>(return_code, return_exception, return_message);
        }
        inline std::vector<Frame*> trace() { return frames; }

        Thread(Frame* frm, CPU *_cpu, decltype(jump_base) jb, Thread* pt): cpu(_cpu), parent_thread(pt), entry_function(frm->function_name),
            debug(false),
            regset(nullptr), uregset(nullptr), tmp(nullptr),
            jump_base(jb),
            frame_new(nullptr), try_frame_new(nullptr),
            thrown(nullptr), caught(nullptr), has_unhandled_exception(false),
            return_value(nullptr),
            return_code(0),
            instruction_counter(0),
            instruction_pointer(nullptr),
            finished(false), is_joinable(true),
            is_suspended(false),
            thread_priority(1)
        {
            uregset = frm->regset;
            frames.push_back(frm);
        }
        ~Thread();
};


#endif
