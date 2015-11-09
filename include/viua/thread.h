#ifndef VIUA_THREAD_H
#define VIUA_THREAD_H

#pragma once

#include <string>
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
    int return_code;                // always set
    std::string return_exception;   // set if CPU stopped because of an exception
    std::string return_message;     // message set by exception

    // FIXME: change unsigned to uint64_t
    unsigned instruction_counter;
    byte* instruction_pointer;

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

    /*  Methods implementing CPU instructions.
     */
    byte* izero(byte*);
    byte* istore(byte*);
    byte* iadd(byte*);
    byte* isub(byte*);
    byte* imul(byte*);
    byte* idiv(byte*);

    byte* ilt(byte*);
    byte* ilte(byte*);
    byte* igt(byte*);
    byte* igte(byte*);
    byte* ieq(byte*);

    byte* iinc(byte*);
    byte* idec(byte*);

    byte* fstore(byte*);
    byte* fadd(byte*);
    byte* fsub(byte*);
    byte* fmul(byte*);
    byte* fdiv(byte*);

    byte* flt(byte*);
    byte* flte(byte*);
    byte* fgt(byte*);
    byte* fgte(byte*);
    byte* feq(byte*);

    byte* bstore(byte*);

    byte* itof(byte*);
    byte* ftoi(byte*);
    byte* stoi(byte*);
    byte* stof(byte*);

    byte* strstore(byte*);

    byte* vec(byte*);
    byte* vinsert(byte*);
    byte* vpush(byte*);
    byte* vpop(byte*);
    byte* vat(byte*);
    byte* vlen(byte*);

    byte* boolean(byte*);
    byte* lognot(byte*);
    byte* logand(byte*);
    byte* logor(byte*);

    byte* move(byte*);
    byte* copy(byte*);
    byte* ref(byte*);
    byte* swap(byte*);
    byte* free(byte*);
    byte* empty(byte*);
    byte* isnull(byte*);

    byte* ress(byte*);
    byte* tmpri(byte*);
    byte* tmpro(byte*);

    byte* print(byte*);
    byte* echo(byte*);

    byte* clbind(byte*);
    byte* closure(byte*);

    byte* function(byte*);
    byte* fcall(byte*);

    byte* frame(byte*);
    byte* param(byte*);
    byte* paref(byte*);
    byte* arg(byte*);
    byte* argc(byte*);

    byte* call(byte*);
    byte* opthread(byte*);
    byte* opthjoin(byte*);
    byte* opthdetach(byte*);
    byte* end(byte*);

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

    byte* import(byte*);
    byte* link(byte*);

    public:
        byte* dispatch(byte*);
        byte* xtick();
        byte* tick();

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
        }

        inline bool stopped() const { return (finished or has_unhandled_exception); }

        inline bool terminated() const { return has_unhandled_exception; }
        inline Type* getActiveException() { return thrown; }

        byte* begin();
        inline unsigned counter() { return instruction_counter; }

        inline std::tuple<int, std::string, std::string> exitcondition() {
            return std::tuple<int, std::string, std::string>(return_code, return_exception, return_message);
        }
        inline std::vector<Frame*> trace() { return frames; }

        Thread(Frame* frm, CPU *_cpu): cpu(_cpu), entry_function(frm->function_name),
            debug(false),
            regset(nullptr), uregset(nullptr), tmp(nullptr),
            jump_base(nullptr),
            frame_new(nullptr), try_frame_new(nullptr),
            thrown(nullptr), caught(nullptr), has_unhandled_exception(false),
            return_code(0),
            instruction_counter(0),
            instruction_pointer(nullptr),
            finished(false), is_joinable(true)
        {
            uregset = frm->regset;
            frames.push_back(frm);
        }
        ~Thread();
};


#endif
