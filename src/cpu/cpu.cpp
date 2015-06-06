#include <iostream>
#include <vector>
#include "../bytecode/bytetypedef.h"
#include "../bytecode/opcodes.h"
#include "../bytecode/maps.h"
#include "../types/type.h"
#include "../types/integer.h"
#include "../types/byte.h"
#include "../types/string.h"
#include "../types/vector.h"
#include "../types/exception.h"
#include "../support/pointer.h"
#include "../include/module.h"
#include "cpu.h"
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

CPU& CPU::bytes(uint16_t sz) {
    /*  Set bytecode size, so the CPU can stop execution even if it doesn't reach HALT instruction but reaches
     *  bytecode address out of bounds.
     */
    bytecode_size = sz;
    return (*this);
}

CPU& CPU::eoffset(uint16_t o) {
    /*  Set offset of first executable instruction.
     */
    executable_offset = o;
    return (*this);
}

CPU& CPU::mapfunction(const string& name, unsigned address) {
    /** Maps function name to bytecode address.
     */
    function_addresses[name] = address;
    return (*this);
}

CPU& CPU::mapblock(const string& name, unsigned address) {
    /** Maps block name to bytecode address.
     */
    block_addresses[name] = address;
    return (*this);
}

CPU& CPU::registerExternalFunction(const string& name, ExternalFunction* function_ptr) {
    /** Registers external function in CPU.
     */
    external_functions[name] = function_ptr;
    return (*this);
}


Type* CPU::fetch(unsigned index) const {
    /*  Return pointer to object at given register.
     *  This method safeguards against reaching for out-of-bounds registers and
     *  reading from an empty register.
     *
     *  :params:
     *
     *  index:int   - index of a register to fetch
     */
    return uregset->get(index);
}


template<class T> inline void copyvalue(Type* a, Type* b) {
    /** This is a short inline, template function to copy value between two `Type` pointers of the same polymorphic type.
     *  It is used internally by CPU.
     */
    static_cast<T>(a)->value() = static_cast<T>(b)->value();
}

void CPU::updaterefs(Type* before, Type* now) {
    /** This method updates references to a given address present in registers.
     *  It swaps old address for the new one in every register that points to the old address.
     *
     *  There is no need to delete old object in this function, as it will be deleted as soon as
     *  it is replaced in the origin register (i.e. the register that holds the original pointer to
     *  the object - the one from which all references had been derived).
     */
    // FIXME: this function should update references in all registersets
    for (unsigned i = 0; i < uregset->size(); ++i) {
        if (uregset->at(i) == before) {
            if (debug) {
                cout << "\nCPU: updating reference address in register " << i << hex << ": " << before << " -> " << now << dec << endl;
            }
            mask_t had_mask = uregset->getmask(i);
            uregset->empty(i);
            uregset->set(i, now);
            uregset->setmask(i, had_mask);
        }
    }
}

bool CPU::hasrefs(unsigned index) {
    /** This method checks if object at a given address exists as a reference in another register.
     */
    bool has = false;
    // FIXME: this should check for references in every register set; gonna be slow, isn't it?
    for (unsigned i = 0; i < uregset->size(); ++i) {
        if (i == index) continue;
        if (uregset->at(i) == uregset->at(index)) {
            has = true;
            break;
        }
    }
    return has;
}

void CPU::place(unsigned index, Type* obj) {
    /** Place an object in register with given index.
     *
     *  Before placing an object in register, a check is preformed if the register is empty.
     *  If not - the `Type` previously stored in it is destroyed.
     *
     */
    Type* old_ref_ptr = (hasrefs(index) ? uregset->at(index) : 0);
    uregset->set(index, obj);

    // update references *if, and only if* the register being set has references and
    // is *not marked a reference* itself, i.e. is the origin register
    if (old_ref_ptr and not uregset->isflagged(index, REFERENCE)) {
        updaterefs(old_ref_ptr, obj);
    }
}

void CPU::ensureStaticRegisters(string function_name) {
    /** Makes sure that static register set for requested function is initialized.
     */
    try {
        static_registers.at(function_name);
    } catch (const std::out_of_range& e) {
        // FIXME: amount of static registers should be customizable
        static_registers[function_name] = new RegisterSet(16);
    }
}


Frame* CPU::requestNewFrame(int arguments_size, int registers_size) {
    /** Request new frame to be prepared.
     *
     *  Creates new frame if the new-frame hook is empty.
     *  Throws an exception otherwise.
     *  Returns pointer to the newly created frame.
     */
    if (frame_new != 0) { throw "requested new frame while last one is unused"; }
    return (frame_new = new Frame(0, arguments_size, registers_size));
}

void CPU::pushFrame() {
    /** Pushes new frame to be the current (top-most) one.
     */
    uregset = frame_new->regset;
    // FIXME: remove this print
    //cout << "\npushing new frame on stack: " << hex << frame_new << dec << " (for function: " << frame_new->function_name << ')' << endl;
    if (find(frames.begin(), frames.end(), frame_new) != frames.end()) {
        ostringstream oss;
        oss << "stack corruption: frame " << hex << frame_new << dec << " for function " << frame_new->function_name << '/' << frame_new->args->size() << " pushed more than once";
        throw oss.str();
    }
    frames.push_back(frame_new);
    frame_new = 0;
}

void CPU::dropFrame() {
    /** Drops top-most frame from call stack.
     */
    // FIXME: remove this print
    //cout << "\ndeleting top frame: " << hex << frames.back() << dec << " (for function: " << frames.back()->function_name << ')' << endl;
    delete frames.back();
    frames.pop_back();

    if (frames.size()) {
        uregset = frames.back()->regset;
    } else {
        uregset = regset;
    }
}


byte* CPU::begin() {
    /** Set instruction pointer to the execution beginning position.
     */
    return (instruction_pointer = bytecode+executable_offset);
}

CPU& CPU::iframe(Frame* frm, unsigned r) {
    /** Set initial frame.
     */
    Frame *initial_frame;
    if (frm == 0) {
        initial_frame = new Frame(0, 0, r);
        initial_frame->function_name = "__entry";

        // set global registers to be the same as __entry function's local registers
        regset = initial_frame->regset;

        Vector* cmdline = new Vector();
        for (unsigned i = 0; i < commandline_arguments.size(); ++i) {
            cmdline->push(new String(commandline_arguments[i]));
        }
        regset->set(1, cmdline);
    } else {
        initial_frame = frm;

        /*  If a frame was supplied to us to be the initial one,
         *  set global registers to the locals of supplied frame.
         */
        delete regset;
        regset = initial_frame->regset;
    }

    // set currently used register set to the global one
    uregset = regset;

    frames.push_back(initial_frame);

    return (*this);
}


byte* CPU::tick() {
    /** Perform a *tick*, i.e. run a single CPU instruction.
     *
     *  Returns pointer to next instruction upon correct execution.
     *  Returns null pointer upon error.
     */
    bool halt = false;
    byte* previous_instruction_pointer = instruction_pointer;
    ++instruction_counter;

    if (debug) {
        cout << "CPU: bytecode ";
        cout << dec << ((long)instruction_pointer - (long)bytecode);
        cout << " at 0x" << hex << (long)instruction_pointer;
        cout << dec << ": ";
    }

    try {
        if (debug) { cout << OP_NAMES.at(OPCODE(*instruction_pointer)); }
        instruction_pointer = dispatch(instruction_pointer);
        if (debug) { cout << endl; }
    } catch (Exception* e) {
        /* All machine-thrown exceptions are passed back to user code.
         * This is much easier than checking for erroneous conditions and
         * terminating functions conditionally, instead - machine just throws Exception objects which
         * are then caught here.
         *
         * If user code cannot deal with them (i.e. did not register a catcher block) they will terminate execution later.
         */
        thrown = e;
    } catch (const HaltException& e) {
        halt = true;
    } catch (const char* e) {
        thrown = new Exception(e);
    }

    if (halt or frames.size() == 0) { return 0; }

    /*  Machine should halt execution if the instruction pointer exceeds bytecode size.
     */
    if (instruction_pointer >= (bytecode+bytecode_size)) {
        return_code = 1;
        return_exception = "InvalidBytecodeAddress";
        return_message = string("instruction address out of bounds");
        return 0;
    }

    /*  Machine should halt execution if previous instruction pointer is the same as current one as
     *  it means that the execution flow is corrupted.
     *
     *  However, execution *should not* be halted if:
     *      - the offending opcode is END (as this may indicate exiting recursive function),
     *      - an object has been thrown, as the instruction pointer will be adjusted by
     *        catchers or execution will be halted on unhandled types,
     */
    if (instruction_pointer == previous_instruction_pointer and OPCODE(*instruction_pointer) != END and thrown == 0) {
        return_code = 2;
        ostringstream oss;
        return_exception = "InstructionUnchanged";
        oss << "instruction pointer did not change, possibly endless loop\n";
        oss << "note: instruction index was " << (long)(instruction_pointer-bytecode) << " and the opcode was '" << OP_NAMES.at(OPCODE(*instruction_pointer)) << "'";
        if (OPCODE(*instruction_pointer) == CALL) {
            oss << '\n';
            oss << "note: this was caused by 'call' opcode immediately calling itself\n"
                << "      such situation may have several sources, e.g. empty function definition or\n"
                << "      a function which calls itself in its first instruction";
        }
        return_message = oss.str();
        return 0;
    }

    TryFrame* tframe;
    // WARNING!
    // This is a temporary hack for more fine-grained exception handling
    if (thrown != 0 and thrown->type() == "Exception") {
        string exception_detailed_type = static_cast<Exception*>(thrown)->etype();
        for (unsigned i = tryframes.size(); i > 0; --i) {
            tframe = tryframes[(i-1)];
            if (tframe->catchers.count(exception_detailed_type)) {
                instruction_pointer = tframe->catchers.at(exception_detailed_type)->block_address;

                caught = thrown;
                thrown = 0;

                break;
            }
        }
    }

    if (thrown != 0) {
        for (unsigned i = tryframes.size(); i > 0; --i) {
            tframe = tryframes[(i-1)];
            if (tframe->catchers.count(thrown->type())) {
                instruction_pointer = tframe->catchers.at(thrown->type())->block_address;

                caught = thrown;
                thrown = 0;

                break;
            }
        }
    }
    if (thrown != 0) {
        // FIXME: catching Type catches everything! (not actually implemented, marked as a TODO)
        vector<string> ic = thrown->inheritancechain();
        for (unsigned i = tryframes.size(); i > 0; --i) {
            tframe = tryframes[(i-1)];
            for (unsigned j = 0; j < ic.size(); ++j) {
                if (tframe->catchers.count(ic[j])) {
                    instruction_pointer = tframe->catchers.at(ic[j])->block_address;

                    caught = thrown;
                    thrown = 0;

                    break;
                }
            }
        }
    }
    if (thrown != 0) {
        return_code = 1;
        return_exception = thrown->type();
        return_message = thrown->repr();
        return 0;
    }

    return instruction_pointer;
}

int CPU::run() {
    /*  VM CPU implementation.
     */
    if (!bytecode) {
        throw "null bytecode (maybe not loaded?)";
    }

    iframe();
    begin(); // set the instruction pointer
    while (tick()) {}

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

    // delete entry function's frame
    // otherwise we get huge memory leak
    // do not delete if execution was halted because of exception
    if (return_exception == "") {
        delete frames.back();
    }

    return return_code;
}
