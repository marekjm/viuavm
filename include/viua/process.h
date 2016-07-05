#ifndef VIUA_PROCESS_H
#define VIUA_PROCESS_H

#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
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
namespace viua {
    namespace scheduler {
        class VirtualProcessScheduler;
    }
}


class Process {
#ifdef AS_DEBUG_HEADER
    public:
#endif
    viua::scheduler::VirtualProcessScheduler *scheduler;

    Process* parent_process;
    const std::string entry_function;

    bool debug;

    // Global register set
    std::unique_ptr<RegisterSet> regset;

    // Currently used register set
    RegisterSet* uregset;

    // Temporary register
    std::unique_ptr<Type> tmp;

    // Static registers
    std::map<std::string, RegisterSet*> static_registers;


    // Call stack
    byte* jump_base;
    std::vector<std::unique_ptr<Frame>> frames;
    std::unique_ptr<Frame> frame_new;

    // Block stack
    std::vector<TryFrame*> tryframes;
    std::unique_ptr<TryFrame> try_frame_new;

    /*  Slot for thrown objects (typically exceptions).
     *  Can be set by user code and the CPU.
     */
    std::unique_ptr<Type> thrown;
    std::unique_ptr<Type> caught;
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
    Type* pop(unsigned);
    void place(unsigned, Type*);
    void updaterefs(Type*, Type*);
    bool hasrefs(unsigned);
    void ensureStaticRegisters(std::string);

    /*  Methods dealing with stack and frame manipulation, and
     *  function calls.
     */
    Frame* requestNewFrame(unsigned arguments_size = 0, unsigned registers_size = 0);
    TryFrame* requestNewTryFrame();
    void pushFrame();
    void dropFrame();
    byte* adjustJumpBaseForBlock(const std::string&);
    byte* adjustJumpBaseFor(const std::string&);
    // call native (i.e. written in Viua) function
    byte* callNative(byte*, const std::string&, const bool, const unsigned, const std::string&);
    // call foreign (i.e. from a C++ extension) function
    byte* callForeign(byte*, const std::string&, const bool, const unsigned, const std::string&);
    // call foreign method (i.e. method of a pure-C++ class loaded into machine's typesystem)
    byte* callForeignMethod(byte*, Type*, const std::string&, const bool, const unsigned, const std::string&);

    /*  Stack unwinding methods.
     */
    void adjustInstructionPointer(TryFrame*, std::string);
    void unwindCallStack(TryFrame*);
    void unwindTryStack(TryFrame*);
    void unwindStack(std::tuple<TryFrame*, std::string>);
    void unwindStack(TryFrame*, std::string);
    std::tuple<TryFrame*, std::string> findCatchFrame();

    bool finished;
    std::atomic_bool is_joinable;
    std::atomic_bool is_suspended;
    unsigned process_priority;
    std::mutex process_mtx;

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
    byte* opptr(byte*);
    byte* opswap(byte*);
    byte* opdelete(byte*);
    byte* opempty(byte*);
    byte* opisnull(byte*);

    byte* opress(byte*);
    byte* optmpri(byte*);
    byte* optmpro(byte*);

    byte* opprint(byte*);
    byte* opecho(byte*);

    byte* openclose(byte*);
    byte* openclosecopy(byte*);
    byte* openclosemove(byte*);
    byte* opclosure(byte*);

    byte* opfunction(byte*);
    byte* opfcall(byte*);

    byte* opframe(byte*);
    byte* opparam(byte*);
    byte* oppamv(byte*);
    byte* oparg(byte*);
    byte* opargc(byte*);

    byte* opcall(byte*);
    byte* optailcall(byte*);
    byte* opprocess(byte*);
    byte* opjoin(byte*);
    byte* opreceive(byte*);
    byte* opwatchdog(byte*);
    byte* opreturn(byte*);

    byte* opjump(byte*);
    byte* opbranch(byte*);

    byte* optry(byte*);
    byte* opcatch(byte*);
    byte* oppull(byte*);
    byte* openter(byte*);
    byte* opthrow(byte*);
    byte* opleave(byte*);

    byte* opclass(byte*);
    byte* opprototype(byte*);
    byte* opderive(byte*);
    byte* opattach(byte*);
    byte* opregister(byte*);

    byte* opnew(byte*);
    byte* opmsg(byte*);
    byte* opinsert(byte*);
    byte* opremove(byte*);

    byte* opimport(byte*);
    byte* oplink(byte*);

    public:
        byte* dispatch(byte*);
        byte* tick();

        Type* obtain(unsigned i) const {
            return fetch(i);
        }
        void put(unsigned i, Type* o) {
            place(i, o);
        }

        bool joinable() const;
        void join();
        void detach();

        void suspend();
        void wakeup();
        bool suspended();

        inline Process* parent() const { return parent_process; };

        void pass(Type* message);

        decltype(process_priority) priority() const { return process_priority; }
        void priority(decltype(process_priority) p) { process_priority = p; }

        inline bool stopped() const { return (finished or has_unhandled_exception); }

        inline bool terminated() const { return has_unhandled_exception; }
        Type* getActiveException();
        Type* transferActiveException();
        void raiseException(Type*);
        void handleActiveException();

        inline void popFrame() {
            dropFrame();
        }
        inline Type* getReturnValue() {
            Type* o = return_value;
            return_value = nullptr;
            return o;
        }

        byte* begin();
        inline uint64_t counter() { return instruction_counter; }
        inline decltype(instruction_pointer) executionAt() { return instruction_pointer; }

        inline std::tuple<int, std::string, std::string> exitcondition() {
            return std::tuple<int, std::string, std::string>(return_code, return_exception, return_message);
        }
        std::vector<Frame*> trace() const;

        Process(std::unique_ptr<Frame>, viua::scheduler::VirtualProcessScheduler*, Process*);
        ~Process();
};


#endif
