#include <iostream>
#include <vector>
#include "../bytecode/bytetypedef.h"
#include "../bytecode/opcodes.h"
#include "../bytecode/maps.h"
#include "../types/object.h"
#include "../types/integer.h"
#include "../types/byte.h"
#include "../support/pointer.h"
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


Object* CPU::fetch(int index) {
    /*  Return pointer to object at given register.
     *  This method safeguards against reaching for out-of-bounds registers and
     *  reading from an empty register.
     *
     *  :params:
     *
     *  index:int   - index of a register to fetch
     */
    if (index >= uregisters_size) { throw "register access out of bounds: read"; }
    Object* optr = uregisters[index];
    if (optr == 0) {
        ostringstream oss;
        oss << "read from null register: " << index;
        throw oss.str().c_str();
    }
    return optr;
}


template<class T> inline void copyvalue(Object* a, Object* b) {
    /** This is a short inline, template function to copy value between two `Object` pointers of the same polymorphic type.
     *  It is used internally by CPU.
     */
    static_cast<T>(a)->value() = static_cast<T>(b)->value();
}

void CPU::updaterefs(Object* before, Object* now) {
    /** This method updates references to a given address present in registers.
     *  It swaps old address for the new one in every register that points to the old address and
     *  is marked as a reference.
     */
    for (int i = 0; i < uregisters_size; ++i) {
        if (uregisters[i] == before and ureferences[i]) {
            if (debug) {
                cout << "\nCPU: updating reference address in register " << i << hex << ": 0x" << (unsigned long)before << " -> 0x" << (unsigned long)now << dec << endl;
            }
            uregisters[i] = now;
        }
    }
}

bool CPU::hasrefs(int index) {
    /** This method checks if object at a given address exists as a reference in another register.
     */
    bool has = false;
    for (int i = 0; i < uregisters_size; ++i) {
        if (i == index) continue;
        if (uregisters[i] == uregisters[index]) {
            has = true;
            break;
        }
    }
    return has;
}

void CPU::place(int index, Object* obj) {
    /** Place an object in register with given index.
     *
     *  Before placing an object in register, a check is preformed if the register is empty.
     *  If not - the `Object` previously stored in it is destroyed.
     *
     */
    if (index >= uregisters_size) { throw "register access out of bounds: write"; }
    if (uregisters[index] != 0 and !ureferences[index]) {
        // register is not empty and is not a reference - the object in it must be destroyed to avoid memory leaks
        delete uregisters[index];
    }
    if (ureferences[index]) {
        Object* referenced = fetch(index);

        // it is a reference, copy value of the object
        if (referenced->type() == "Integer") { copyvalue<Integer*>(referenced, obj); }
        else if (referenced->type() == "Byte") { copyvalue<Byte*>(referenced, obj); }

        // and delete the newly created object to avoid leaks
        delete obj;
    } else {
        Object* old_ref_ptr = (hasrefs(index) ? uregisters[index] : 0);
        uregisters[index] = obj;
        if (old_ref_ptr) { updaterefs(old_ref_ptr, obj); }
    }
}

void CPU::ensureStaticRegisters(string function_name) {
    /** Makes sure that static register set for requested function is initialized.
     */
    try {
        static_registers.at(function_name);
    } catch (const std::out_of_range& e) {
        // 1) initialise all register set variables
        int s_registers_size = 16;  // FIXME: size of static register should be customisable
        Object** s_registers = new Object*[s_registers_size];
        bool* s_references = new bool[s_registers_size];

        // 2) zero registers and refrences
        for (int i = 0; i < s_registers_size; ++i) {
            s_registers[i] = 0;
            s_references[i] = false;
        }

        // 3) assign initialised register set to a function name
        static_registers[function_name] = tuple<Object**, bool*, int>(s_registers, s_references, s_registers_size);
    }
}

byte* CPU::begin() {
    /** Set instruction pointer to the execution beginning position.
     */
    return (instruction_pointer = bytecode+executable_offset);
}

CPU& CPU::iframe(Frame* frm) {
    /** Set initial frame.
     */
    Frame *initial_frame;
    if (frm == 0) {
        initial_frame = new Frame(0, 0, 0);
        initial_frame->registers = registers;
        initial_frame->references = references;
        initial_frame->registers_size = reg_count;
        initial_frame->function_name = "__entry";
    } else {
        initial_frame = frm;
    }
    uregisters = initial_frame->registers;
    ureferences = initial_frame->references;
    uregisters_size = initial_frame->registers_size;
    frames.push_back(initial_frame);
    return (*this);
}

byte* CPU::tick() {
    /** Perform a *tick*, i.e. run a single CPU instruction.
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
        switch (*instruction_pointer) {
            case IZERO:
                instruction_pointer = izero(instruction_pointer+1);
                break;
            case ISTORE:
                instruction_pointer = istore(instruction_pointer+1);
                break;
            case IADD:
                instruction_pointer = iadd(instruction_pointer+1);
                break;
            case ISUB:
                instruction_pointer = isub(instruction_pointer+1);
                break;
            case IMUL:
                instruction_pointer = imul(instruction_pointer+1);
                break;
            case IDIV:
                instruction_pointer = idiv(instruction_pointer+1);
                break;
            case IINC:
                instruction_pointer = iinc(instruction_pointer+1);
                break;
            case IDEC:
                instruction_pointer = idec(instruction_pointer+1);
                break;
            case ILT:
                instruction_pointer = ilt(instruction_pointer+1);
                break;
            case ILTE:
                instruction_pointer = ilte(instruction_pointer+1);
                break;
            case IGT:
                instruction_pointer = igt(instruction_pointer+1);
                break;
            case IGTE:
                instruction_pointer = igte(instruction_pointer+1);
                break;
            case IEQ:
                instruction_pointer = ieq(instruction_pointer+1);
                break;
            case FSTORE:
                instruction_pointer = fstore(instruction_pointer+1);
                break;
            case FADD:
                instruction_pointer = fadd(instruction_pointer+1);
                break;
            case FSUB:
                instruction_pointer = fsub(instruction_pointer+1);
                break;
            case FMUL:
                instruction_pointer = fmul(instruction_pointer+1);
                break;
            case FDIV:
                instruction_pointer = fdiv(instruction_pointer+1);
                break;
            case FLT:
                instruction_pointer = flt(instruction_pointer+1);
                break;
            case FLTE:
                instruction_pointer = flte(instruction_pointer+1);
                break;
            case FGT:
                instruction_pointer = fgt(instruction_pointer+1);
                break;
            case FGTE:
                instruction_pointer = fgte(instruction_pointer+1);
                break;
            case FEQ:
                instruction_pointer = feq(instruction_pointer+1);
                break;
            case BSTORE:
                instruction_pointer = bstore(instruction_pointer+1);
                break;
            case ITOF:
                instruction_pointer = itof(instruction_pointer+1);
                break;
            case FTOI:
                instruction_pointer = ftoi(instruction_pointer+1);
                break;
            case STRSTORE:
                instruction_pointer = strstore(instruction_pointer+1);
                break;
            case NOT:
                instruction_pointer = lognot(instruction_pointer+1);
                break;
            case AND:
                instruction_pointer = logand(instruction_pointer+1);
                break;
            case OR:
                instruction_pointer = logor(instruction_pointer+1);
                break;
            case MOVE:
                instruction_pointer = move(instruction_pointer+1);
                break;
            case COPY:
                instruction_pointer = copy(instruction_pointer+1);
                break;
            case REF:
                instruction_pointer = ref(instruction_pointer+1);
                break;
            case SWAP:
                instruction_pointer = swap(instruction_pointer+1);
                break;
            case FREE:
                instruction_pointer = free(instruction_pointer+1);
                break;
            case EMPTY:
                instruction_pointer = empty(instruction_pointer+1);
                break;
            case ISNULL:
                instruction_pointer = isnull(instruction_pointer+1);
                break;
            case RESS:
                instruction_pointer = ress(instruction_pointer+1);
                break;
            case TMPRI:
                instruction_pointer = tmpri(instruction_pointer+1);
                break;
            case TMPRO:
                instruction_pointer = tmpro(instruction_pointer+1);
                break;
            case PRINT:
                instruction_pointer = print(instruction_pointer+1);
                break;
            case ECHO:
                instruction_pointer = echo(instruction_pointer+1);
                break;
            case FRAME:
                instruction_pointer = frame(instruction_pointer+1);
                break;
            case PARAM:
                instruction_pointer = param(instruction_pointer+1);
                break;
            case PAREF:
                instruction_pointer = paref(instruction_pointer+1);
                break;
            case ARG:
                instruction_pointer = arg(instruction_pointer+1);
                break;
            case CALL:
                instruction_pointer = call(instruction_pointer+1);
                break;
            case END:
                instruction_pointer = end(instruction_pointer);
                break;
            case JUMP:
                instruction_pointer = jump(instruction_pointer+1);
                break;
            case BRANCH:
                instruction_pointer = branch(instruction_pointer+1);
                break;
            case HALT:
                halt = true;
                break;
            case PASS:
                ++instruction_pointer;
                break;
            case NOP:
                ++instruction_pointer;
                break;
            default:
                ostringstream error;
                error << "unrecognised instruction (bytecode value: " << *((int*)bytecode) << ")";
                throw error.str().c_str();
        }
        if (debug and not stepping) { cout << endl; }
    } catch (const char*& e) {
        return_code = 1;
        return_message = string(e);
        return_exception = "RuntimeException";
        return 0;
    } catch (const string& e) {
        return_code = 1;
        return_message = string(e);
        return_exception =  "RuntimeException";
        return 0;
    }

    if (halt or frames.size() == 0) { return 0; }

    if (instruction_pointer >= (bytecode+bytecode_size)) {
        return_code = 1;
        return_exception = "InvalidBytecodeAddress";
        return_message = string("instruction address out of bounds");
        return 0;
    }

    if (instruction_pointer == previous_instruction_pointer and OPCODE(*instruction_pointer) != END) {
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

    if (stepping) {
        string ins;
        getline(cin, ins);
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

    if (return_code == 0 and uregisters[0]) {
        // if return code if the default one and
        // return register is not unused
        // copy value of return register as return code
        try {
            return_code = static_cast<Integer*>(uregisters[0])->value();
        } catch (const char*& e) {
            return_code = 1;
            return_exception = "ReturnStageException";
            return_message = string(e);
        } catch (const string& e) {
            return_code = 1;
            return_exception = "ReturnStageException";
            return_message = string(e);
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
