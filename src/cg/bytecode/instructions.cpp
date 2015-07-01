#include "instructions.h"
using namespace std;


byte* insertIntegerOperand(byte* addr_ptr, int_op op) {
    /** Insert integer operand into bytecode.
     *
     *  When using integer operand, it usually is a plain number - which translates to a regsiter index.
     *  However, when preceded by `@` integer operand will not be interpreted directly, but instead CPU
     *  will look into a register the integer points to, fetch an integer from this register and
     *  use the fetched register as the operand.
     */
    bool ref;
    int num;

    tie(ref, num) = op;

    *((bool*)addr_ptr) = ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = num;
    pointer::inc<int, byte>(addr_ptr);

    return addr_ptr;
}

byte* insertTwoIntegerOpsInstruction(byte* addr_ptr, enum OPCODE instruction, int_op a, int_op b) {
    /** Insert instruction with two integer operands.
     */
    *(addr_ptr++) = instruction;
    addr_ptr = insertIntegerOperand(addr_ptr, a);
    addr_ptr = insertIntegerOperand(addr_ptr, b);
    return addr_ptr;
}

byte* insertThreeIntegerOpsInstruction(byte* addr_ptr, enum OPCODE instruction, int_op a, int_op b, int_op c) {
    /** Insert instruction with two integer operands.
     */
    *(addr_ptr++) = instruction;
    addr_ptr = insertIntegerOperand(addr_ptr, a);
    addr_ptr = insertIntegerOperand(addr_ptr, b);
    addr_ptr = insertIntegerOperand(addr_ptr, c);
    return addr_ptr;
}


namespace cg {
    namespace bytecode {
        byte* nop(byte* addr_ptr) {
            /*  Inserts nop instuction.
             */
            *(addr_ptr++) = NOP;
            return addr_ptr;
        }

        byte* izero(byte* addr_ptr, int_op regno) {
            /*  Inserts izero instuction.
             */
            *(addr_ptr++) = IZERO;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            return addr_ptr;
        }

        byte* istore(byte* addr_ptr, int_op regno, int_op i) {
            /*  Inserts istore instruction to bytecode.
             *
             *  :params:
             *
             *  regno:int - register number
             *  i:int     - value to store
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, ISTORE, regno, i);
            return addr_ptr;
        }

        byte* iadd(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts iadd instruction to bytecode.
             *
             *  :params:
             *
             *  rega    - register index of first operand
             *  regb    - register index of second operand
             *  regr    - register index in which to store the result
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, IADD, rega, regb, regr);
            return addr_ptr;
        }

        byte* isub(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts isub instruction to bytecode.
             *
             *  :params:
             *
             *  rega    - register index of first operand
             *  regb    - register index of second operand
             *  regr    - register index in which to store the result
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, ISUB, rega, regb, regr);
            return addr_ptr;
        }

        byte* imul(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts imul instruction to bytecode.
             *
             *  :params:
             *
             *  rega    - register index of first operand
             *  regb    - register index of second operand
             *  regr    - register index in which to store the result
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, IMUL, rega, regb, regr);
            return addr_ptr;
        }

        byte* idiv(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts idiv instruction to bytecode.
             *
             *  :params:
             *
             *  rega    - register index of first operand
             *  regb    - register index of second operand
             *  regr    - register index in which to store the result
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, IDIV, rega, regb, regr);
            return addr_ptr;
        }

        byte* iinc(byte* addr_ptr, int_op regno) {
            /*  Inserts iinc instuction.
             */
            *(addr_ptr++) = IINC;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            return addr_ptr;
        }

        byte* idec(byte* addr_ptr, int_op regno) {
            /*  Inserts idec instuction.
             */
            *(addr_ptr++) = IDEC;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            return addr_ptr;
        }

        byte* ilt(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts ilt instruction to bytecode.
             *
             *  :params:
             *
             *  rega    - register index of first operand
             *  regb    - register index of second operand
             *  regr    - register index in which to store the result
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, ILT, rega, regb, regr);
            return addr_ptr;
        }

        byte* ilte(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts ilte instruction to bytecode.
             *
             *  :params:
             *
             *  rega    - register index of first operand
             *  regb    - register index of second operand
             *  regr    - register index in which to store the result
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, ILTE, rega, regb, regr);
            return addr_ptr;
        }

        byte* igt(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts igt instruction to bytecode.
             *
             *  :params:
             *
             *  rega    - register index of first operand
             *  regb    - register index of second operand
             *  regr    - register index in which to store the result
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, IGT, rega, regb, regr);
            return addr_ptr;
        }

        byte* igte(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts igte instruction to bytecode.
             *
             *  :params:
             *
             *  rega    - register index of first operand
             *  regb    - register index of second operand
             *  regr    - register index in which to store the result
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, IGTE, rega, regb, regr);
            return addr_ptr;
        }

        byte* ieq(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts ieq instruction to bytecode.
             *
             *  :params:
             *
             *  rega    - register index of first operand
             *  regb    - register index of second operand
             *  regr    - register index in which to store the result
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, IEQ, rega, regb, regr);
            return addr_ptr;
        }

        byte* fstore(byte* addr_ptr, int_op regno, float f) {
            /*  Inserts fstore instruction to bytecode.
             *
             *  :params:
             *
             *  regno - register number
             *  f     - value to store
             */
            *(addr_ptr++) = FSTORE;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            *((float*)addr_ptr)  = f;
            pointer::inc<float, byte>(addr_ptr);

            return addr_ptr;
        }

        byte* fadd(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts fadd instruction to bytecode.
             *
             *  :params:
             *
             *  rega    - register index of first operand
             *  regb    - register index of second operand
             *  regr    - register index in which to store the result
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FADD, rega, regb, regr);
            return addr_ptr;
        }

        byte* fsub(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts fsub instruction to bytecode.
             *
             *  :params:
             *
             *  rega    - register index of first operand
             *  regb    - register index of second operand
             *  regr    - register index in which to store the result
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FSUB, rega, regb, regr);
            return addr_ptr;
        }

        byte* fmul(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts fmul instruction to bytecode.
             *
             *  :params:
             *
             *  rega    - register index of first operand
             *  regb    - register index of second operand
             *  regr    - register index in which to store the result
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FMUL, rega, regb, regr);
            return addr_ptr;
        }

        byte* fdiv(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts fdiv instruction to bytecode.
             *
             *  :params:
             *
             *  rega    - register index of first operand
             *  regb    - register index of second operand
             *  regr    - register index in which to store the result
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FDIV, rega, regb, regr);
            return addr_ptr;
        }

        byte* flt(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts flt instruction to bytecode.
             *
             *  :params:
             *
             *  rega    - register index of first operand
             *  regb    - register index of second operand
             *  regr    - register index in which to store the result
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FLT, rega, regb, regr);
            return addr_ptr;
        }

        byte* flte(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts flte instruction to bytecode.
             *
             *  :params:
             *
             *  rega    - register index of first operand
             *  regb    - register index of second operand
             *  regr    - register index in which to store the result
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FLTE, rega, regb, regr);
            return addr_ptr;
        }

        byte* fgt(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts fgt instruction to bytecode.
             *
             *  :params:
             *
             *  rega    - register index of first operand
             *  regb    - register index of second operand
             *  regr    - register index in which to store the resugt
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FGT, rega, regb, regr);
            return addr_ptr;
        }

        byte* fgte(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts fgte instruction to bytecode.
             *
             *  :params:
             *
             *  rega    - register index of first operand
             *  regb    - register index of second operand
             *  regr    - register index in which to store the resugt
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FGTE, rega, regb, regr);
            return addr_ptr;
        }

        byte* feq(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts feq instruction to bytecode.
             *
             *  :params:
             *
             *  rega    - register index of first operand
             *  regb    - register index of second operand
             *  regr    - register index in which to store the resugt
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FEQ, rega, regb, regr);
            return addr_ptr;
        }

        byte* bstore(byte* addr_ptr, int_op regno, byte_op b) {
            /*  Inserts bstore instruction to bytecode.
             *
             *  :params:
             *
             *  regno - register number
             *  b     - value to store
             */
            bool b_ref = false;
            byte bt;

            tie(b_ref, bt) = b;

            *(addr_ptr++) = BSTORE;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            *((bool*)addr_ptr) = b_ref;
            pointer::inc<bool, byte>(addr_ptr);
            *((byte*)addr_ptr)  = bt;
            ++addr_ptr;

            return addr_ptr;
        }

        byte* itof(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts itof instruction to bytecode.
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, ITOF, a, b);
            return addr_ptr;
        }

        byte* ftoi(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts ftoi instruction to bytecode.
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, FTOI, a, b);
            return addr_ptr;
        }

        byte* stoi(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts stoi instruction to bytecode.
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, STOI, a, b);
            return addr_ptr;
        }

        byte* stof(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts stof instruction to bytecode.
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, STOF, a, b);
            return addr_ptr;
        }

        byte* strstore(byte* addr_ptr, int_op reg, string s) {
            /*  Inserts strstore instruction.
             */
            *(addr_ptr++) = STRSTORE;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);

            for (unsigned i = 1; i < s.size()-1; ++i) {
                *((char*)addr_ptr++) = s[i];
            }
            *((char*)addr_ptr++) = char(0);
            return addr_ptr;
        }

        byte* vec(byte* addr_ptr, int_op index) {
            /** Inserts vec instruction.
             */
            *(addr_ptr++) = VEC;
            addr_ptr = insertIntegerOperand(addr_ptr, index);
            return addr_ptr;
        }

        byte* vinsert(byte* addr_ptr, int_op vec, int_op src, int_op dst) {
            /** Inserts vinsert instruction.
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, VINSERT, vec, src, dst);
            return addr_ptr;
        }

        byte* vpush(byte* addr_ptr, int_op vec, int_op src) {
            /** Inserts vpush instruction.
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, VPUSH, vec, src);
            return addr_ptr;
        }

        byte* vpop(byte* addr_ptr, int_op vec, int_op dst, int_op pos) {
            /** Inserts vpop instruction.
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, VPOP, vec, dst, pos);
            return addr_ptr;
        }

        byte* vat(byte* addr_ptr, int_op vec, int_op dst, int_op at) {
            /** Inserts vat instruction.
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, VAT, vec, dst, at);
            return addr_ptr;
        }

        byte* vlen(byte* addr_ptr, int_op vec, int_op reg) {
            /** Inserts vlen instruction.
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, VLEN, vec, reg);
            return addr_ptr;
        }

        byte* lognot(byte* addr_ptr, int_op reg) {
            /*  Inserts not instuction.
             */
            *(addr_ptr++) = NOT;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* logand(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts and instruction to bytecode.
             *
             *  :params:
             *
             *  rega   - register index of first operand
             *  regb   - register index of second operand
             *  regr   - register index in which to store the result
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, AND, rega, regb, regr);
            return addr_ptr;
        }

        byte* logor(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            /*  Inserts or instruction to bytecode.
             *
             *  :params:
             *
             *  rega   - register index of first operand
             *  regb   - register index of second operand
             *  regr   - register index in which to store the result
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, OR, rega, regb, regr);
            return addr_ptr;
        }

        byte* move(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts move instruction to bytecode.
             *
             *  :params:
             *
             *  a - register number (move from...)
             *  b - register number (move to...)
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, MOVE, a, b);
            return addr_ptr;
        }

        byte* copy(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts copy instruction to bytecode.
             *
             *  :params:
             *
             *  a - register number (copy from...)
             *  b - register number (copy to...)
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, COPY, a, b);
            return addr_ptr;
        }

        byte* ref(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts ref instruction to bytecode.
             *
             *  :params:
             *
             *  a - register number
             *  b - register number
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, REF, a, b);
            return addr_ptr;
        }

        byte* swap(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts swap instruction to bytecode.
             *
             *  :params:
             *
             *  a - register number
             *  b - register number
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, SWAP, a, b);
            return addr_ptr;
        }

        byte* free(byte* addr_ptr, int_op reg) {
            /*  Inserts free instuction.
             */
            *(addr_ptr++) = FREE;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* empty(byte* addr_ptr, int_op reg) {
            /*  Inserts empty instuction.
             */
            *(addr_ptr++) = EMPTY;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* isnull(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts isnull instruction to bytecode.
             *
             *  :params:
             *
             *  a - register number
             *  b - register number
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, ISNULL, a, b);
            return addr_ptr;
        }

        byte* ress(byte* addr_ptr, const string& a) {
            /*  Inserts ress instruction to bytecode.
             *
             *  :params:
             *
             *  a - register set ID
             */
            *(addr_ptr++) = RESS;
            if (a == "global") {
                *((int*)addr_ptr) = 0;
            } else if (a == "local") {
                *((int*)addr_ptr) = 1;
            } else if (a == "static") {
                *((int*)addr_ptr) = 2;
            } else if (a == "temp") {
                *((int*)addr_ptr) = 3;
            }
            pointer::inc<int, byte>(addr_ptr);
            return addr_ptr;
        }

        byte* tmpri(byte* addr_ptr, int_op reg) {
            /*  Inserts tmpri instuction.
             */
            *(addr_ptr++) = TMPRI;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* tmpro(byte* addr_ptr, int_op reg) {
            /*  Inserts tmpro instuction.
             */
            *(addr_ptr++) = TMPRO;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* print(byte* addr_ptr, int_op reg) {
            /*  Inserts print instuction.
             */
            *(addr_ptr++) = PRINT;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* echo(byte* addr_ptr, int_op reg) {
            /*  Inserts echo instuction.
             */
            *(addr_ptr++) = ECHO;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* clbind(byte* addr_ptr, int_op reg) {
            /*  Inserts clbing instuction.
             */
            *(addr_ptr++) = CLBIND;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* closure(byte* addr_ptr, const string& fn, int_op reg) {
            /*  Inserts closure instuction.
             */
            *(addr_ptr++) = CLOSURE;
            for (unsigned i = 0; i < fn.size(); ++i) {
                *((char*)addr_ptr++) = fn[i];
            }
            *(addr_ptr++) = '\0';
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* function(byte* addr_ptr, const string& fn, int_op reg) {
            /*  Inserts function instuction.
             */
            *(addr_ptr++) = FUNCTION;
            for (unsigned i = 0; i < fn.size(); ++i) {
                *((char*)addr_ptr++) = fn[i];
            }
            *(addr_ptr++) = '\0';
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* fcall(byte* addr_ptr, int_op clsr, int_op ret) {
            /*  Inserts fcall instruction to bytecode.
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, FCALL, clsr, ret);
            return addr_ptr;
        }

        byte* frame(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts frame instruction to bytecode.
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, FRAME, a, b);
            return addr_ptr;
        }

        byte* param(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts param instruction to bytecode.
             *
             *  :params:
             *
             *  a - register number
             *  b - register number
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, PARAM, a, b);
            return addr_ptr;
        }

        byte* paref(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts paref instruction to bytecode.
             *
             *  :params:
             *
             *  a - register number
             *  b - register number
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, PAREF, a, b);
            return addr_ptr;
        }

        byte* arg(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts arg instruction to bytecode.
             *
             *  :params:
             *
             *  a - argument number
             *  b - register number
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, ARG, a, b);
            return addr_ptr;
        }

        byte* call(byte* addr_ptr, const string& fn_name, int_op reg) {
            /*  Inserts call instruction.
             *  Byte offset is calculated automatically.
             */
            *(addr_ptr++) = CALL;
            for (unsigned i = 0; i < fn_name.size(); ++i) {
                *((char*)addr_ptr++) = fn_name[i];
            }
            *(addr_ptr++) = '\0';
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* jump(byte* addr_ptr, int addr, enum JUMPTYPE is_absolute) {
            /*  Inserts jump instruction. Parameter is instruction index.
             *  Byte offset is calculated automatically.
             *
             *  :params:
             *
             *  addr:int    - index of the instruction to which to branch
             */
            *(addr_ptr++) = JUMP;

            *((int*)addr_ptr) = addr;
            pointer::inc<int, byte>(addr_ptr);

            return addr_ptr;
        }

        byte* branch(byte* addr_ptr, int_op regc, int addr_truth, int addr_false) {
            /*  Inserts branch instruction.
             *  Byte offset is calculated automatically.
             */
            *(addr_ptr++) = BRANCH;
            addr_ptr = insertIntegerOperand(addr_ptr, regc);

            *((int*)addr_ptr) = addr_truth;
            pointer::inc<int, byte>(addr_ptr);

            *((int*)addr_ptr) = addr_false;
            pointer::inc<int, byte>(addr_ptr);

            return addr_ptr;
        }

        byte* tryframe(byte* addr_ptr) {
            /*  Inserts tryframe instruction.
             */
            *(addr_ptr++) = TRYFRAME;
            return addr_ptr;
        }

        byte* vmcatch(byte* addr_ptr, const string& type_name, const string& block_name) {
            /*  Inserts catch instruction.
             */
            *(addr_ptr++) = CATCH;

            // the type
            for (unsigned i = 1; i < type_name.size()-1; ++i) {
                *((char*)addr_ptr++) = type_name[i];
            }
            *((char*)addr_ptr++) = '\0';

            // catcher block name
            for (unsigned i = 0; i < block_name.size(); ++i) {
                *((char*)addr_ptr++) = block_name[i];
            }
            *((char*)addr_ptr++) = '\0';

            return addr_ptr;
        }

        byte* pull(byte* addr_ptr, int_op regno) {
            /*  Inserts throw instuction.
             */
            *(addr_ptr++) = PULL;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            return addr_ptr;
        }

        byte* vmtry(byte* addr_ptr, const string& block_name) {
            /*  Inserts try instruction.
             *  Byte offset is calculated automatically.
             */
            *(addr_ptr++) = TRY;
            for (unsigned i = 0; i < block_name.size(); ++i) {
                *((char*)addr_ptr++) = block_name[i];
            }
            *(addr_ptr++) = '\0';
            return addr_ptr;
        }

        byte* vmthrow(byte* addr_ptr, int_op regno) {
            /*  Inserts throw instuction.
             */
            *(addr_ptr++) = THROW;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            return addr_ptr;
        }

        byte* leave(byte* addr_ptr) {
            /*  Inserts leave instruction.
             */
            *(addr_ptr++) = LEAVE;
            return addr_ptr;
        }

        byte* eximport(byte* addr_ptr, const string& module_name) {
            /*  Inserts eximport instruction.
             */
            *(addr_ptr++) = EXIMPORT;
            for (unsigned i = 1; i < module_name.size()-1; ++i) {
                *((char*)addr_ptr++) = module_name[i];
            }
            *((char*)addr_ptr++) = char(0);
            return addr_ptr;
        }

        byte* excall(byte* addr_ptr, const string& fn_name, int_op reg) {
            /*  Inserts excall instruction.
             *  Byte offset is calculated automatically.
             */
            *(addr_ptr++) = EXCALL;
            for (unsigned i = 0; i < fn_name.size(); ++i) {
                *((char*)addr_ptr++) = fn_name[i];
            }
            *(addr_ptr++) = '\0';
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* end(byte* addr_ptr) {
            /*  Inserts end instruction.
             */
            *(addr_ptr++) = END;
            return addr_ptr;
        }

        byte* halt(byte* addr_ptr) {
            /*  Inserts halt instruction.
             */
            *(addr_ptr++) = HALT;
            return addr_ptr;
        }
    }
}
