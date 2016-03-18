#include <viua/cg/bytecode/instructions.h>
#include <viua/bytecode/operand_types.h>
using namespace std;


static byte* insertIntegerOperand(byte* addr_ptr, int_op op) {
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

    if (ref) {
        *((OperandType*)addr_ptr) = OT_REGISTER_REFERENCE;
    } else {
        *((OperandType*)addr_ptr) = OT_REGISTER_INDEX;
    }
    pointer::inc<OperandType, byte>(addr_ptr);
    *((int*)addr_ptr)  = num;
    pointer::inc<int, byte>(addr_ptr);

    return addr_ptr;
}

static byte* insertTwoIntegerOpsInstruction(byte* addr_ptr, enum OPCODE instruction, int_op a, int_op b) {
    /** Insert instruction with two integer operands.
     */
    *(addr_ptr++) = instruction;
    addr_ptr = insertIntegerOperand(addr_ptr, a);
    addr_ptr = insertIntegerOperand(addr_ptr, b);
    return addr_ptr;
}

static byte* insertThreeIntegerOpsInstruction(byte* addr_ptr, enum OPCODE instruction, int_op a, int_op b, int_op c) {
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
        byte* opnop(byte* addr_ptr) {
            /*  Inserts nop instuction.
             */
            *(addr_ptr++) = NOP;
            return addr_ptr;
        }

        byte* opizero(byte* addr_ptr, int_op regno) {
            /*  Inserts izero instuction.
             */
            *(addr_ptr++) = IZERO;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            return addr_ptr;
        }

        byte* opistore(byte* addr_ptr, int_op regno, int_op i) {
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

        byte* opiadd(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
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

        byte* opisub(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
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

        byte* opimul(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
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

        byte* opidiv(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
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

        byte* opiinc(byte* addr_ptr, int_op regno) {
            /*  Inserts iinc instuction.
             */
            *(addr_ptr++) = IINC;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            return addr_ptr;
        }

        byte* opidec(byte* addr_ptr, int_op regno) {
            /*  Inserts idec instuction.
             */
            *(addr_ptr++) = IDEC;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            return addr_ptr;
        }

        byte* opilt(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
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

        byte* opilte(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
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

        byte* opigt(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
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

        byte* opigte(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
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

        byte* opieq(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
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

        byte* opfstore(byte* addr_ptr, int_op regno, float f) {
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

        byte* opfadd(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
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

        byte* opfsub(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
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

        byte* opfmul(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
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

        byte* opfdiv(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
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

        byte* opflt(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
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

        byte* opflte(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
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

        byte* opfgt(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
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

        byte* opfgte(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
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

        byte* opfeq(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
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

        byte* opbstore(byte* addr_ptr, int_op regno, byte_op b) {
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
            *(addr_ptr) = bt;
            ++addr_ptr;

            return addr_ptr;
        }

        byte* opitof(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts itof instruction to bytecode.
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, ITOF, a, b);
            return addr_ptr;
        }

        byte* opftoi(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts ftoi instruction to bytecode.
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, FTOI, a, b);
            return addr_ptr;
        }

        byte* opstoi(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts stoi instruction to bytecode.
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, STOI, a, b);
            return addr_ptr;
        }

        byte* opstof(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts stof instruction to bytecode.
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, STOF, a, b);
            return addr_ptr;
        }

        byte* opstrstore(byte* addr_ptr, int_op reg, string s) {
            /*  Inserts strstore instruction.
             */
            *(addr_ptr++) = STRSTORE;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);

            for (unsigned i = 1; i < s.size()-1; ++i) {
                *(addr_ptr++) = s[i];
            }
            *(addr_ptr++) = char(0);
            return addr_ptr;
        }

        byte* opvec(byte* addr_ptr, int_op index) {
            /** Inserts vec instruction.
             */
            *(addr_ptr++) = VEC;
            addr_ptr = insertIntegerOperand(addr_ptr, index);
            return addr_ptr;
        }

        byte* opvinsert(byte* addr_ptr, int_op vec, int_op src, int_op dst) {
            /** Inserts vinsert instruction.
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, VINSERT, vec, src, dst);
            return addr_ptr;
        }

        byte* opvpush(byte* addr_ptr, int_op vec, int_op src) {
            /** Inserts vpush instruction.
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, VPUSH, vec, src);
            return addr_ptr;
        }

        byte* opvpop(byte* addr_ptr, int_op vec, int_op dst, int_op pos) {
            /** Inserts vpop instruction.
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, VPOP, vec, dst, pos);
            return addr_ptr;
        }

        byte* opvat(byte* addr_ptr, int_op vec, int_op dst, int_op at) {
            /** Inserts vat instruction.
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, VAT, vec, dst, at);
            return addr_ptr;
        }

        byte* opvlen(byte* addr_ptr, int_op vec, int_op reg) {
            /** Inserts vlen instruction.
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, VLEN, vec, reg);
            return addr_ptr;
        }

        byte* opnot(byte* addr_ptr, int_op reg) {
            /*  Inserts not instuction.
             */
            *(addr_ptr++) = NOT;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* opand(byte* addr_ptr, int_op regr, int_op rega, int_op regb) {
            /*  Inserts and instruction to bytecode.
             *
             *  :params:
             *
             *  regr   - register index in which to store the result
             *  rega   - register index of first operand
             *  regb   - register index of second operand
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, AND, regr, rega, regb);
            return addr_ptr;
        }

        byte* opor(byte* addr_ptr, int_op regr, int_op rega, int_op regb) {
            /*  Inserts or instruction to bytecode.
             *
             *  :params:
             *
             *  regr   - register index in which to store the result
             *  rega   - register index of first operand
             *  regb   - register index of second operand
             */
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, OR, regr, rega, regb);
            return addr_ptr;
        }

        byte* opmove(byte* addr_ptr, int_op a, int_op b) {
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

        byte* opcopy(byte* addr_ptr, int_op a, int_op b) {
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

        byte* opref(byte* addr_ptr, int_op a, int_op b) {
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

        byte* opptr(byte* addr_ptr, int_op a, int_op b) {
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, PTR, a, b);
            return addr_ptr;
        }

        byte* opswap(byte* addr_ptr, int_op a, int_op b) {
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

        byte* opdelete(byte* addr_ptr, int_op reg) {
            *(addr_ptr++) = DELETE;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* opempty(byte* addr_ptr, int_op reg) {
            /*  Inserts empty instuction.
             */
            *(addr_ptr++) = EMPTY;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* opisnull(byte* addr_ptr, int_op a, int_op b) {
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

        byte* opress(byte* addr_ptr, const string& a) {
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

        byte* optmpri(byte* addr_ptr, int_op reg) {
            /*  Inserts tmpri instuction.
             */
            *(addr_ptr++) = TMPRI;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* optmpro(byte* addr_ptr, int_op reg) {
            /*  Inserts tmpro instuction.
             */
            *(addr_ptr++) = TMPRO;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* opprint(byte* addr_ptr, int_op reg) {
            /*  Inserts print instuction.
             */
            *(addr_ptr++) = PRINT;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* opecho(byte* addr_ptr, int_op reg) {
            /*  Inserts echo instuction.
             */
            *(addr_ptr++) = ECHO;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* openclose(byte* addr_ptr, int_op target_closure, int_op target_register, int_op source_register) {
            /*  Inserts clbing instuction.
             */
            *(addr_ptr++) = ENCLOSE;
            addr_ptr = insertIntegerOperand(addr_ptr, target_closure);
            addr_ptr = insertIntegerOperand(addr_ptr, target_register);
            addr_ptr = insertIntegerOperand(addr_ptr, source_register);
            return addr_ptr;
        }

        byte* openclosecopy(byte* addr_ptr, int_op target_closure, int_op target_register, int_op source_register) {
            /*  Inserts enclosecopy instuction.
             */
            *(addr_ptr++) = ENCLOSECOPY;
            addr_ptr = insertIntegerOperand(addr_ptr, target_closure);
            addr_ptr = insertIntegerOperand(addr_ptr, target_register);
            addr_ptr = insertIntegerOperand(addr_ptr, source_register);
            return addr_ptr;
        }

        byte* openclosemove(byte* addr_ptr, int_op target_closure, int_op target_register, int_op source_register) {
            /*  Inserts openclosemove instuction.
             */
            *(addr_ptr++) = ENCLOSEMOVE;
            addr_ptr = insertIntegerOperand(addr_ptr, target_closure);
            addr_ptr = insertIntegerOperand(addr_ptr, target_register);
            addr_ptr = insertIntegerOperand(addr_ptr, source_register);
            return addr_ptr;
        }

        byte* opclosure(byte* addr_ptr, int_op reg, const string& fn) {
            /*  Inserts closure instuction.
             */
            *(addr_ptr++) = CLOSURE;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            for (unsigned i = 0; i < fn.size(); ++i) {
                *(addr_ptr++) = fn[i];
            }
            *(addr_ptr++) = '\0';
            return addr_ptr;
        }

        byte* opfunction(byte* addr_ptr, int_op reg, const string& fn) {
            /*  Inserts function instuction.
             */
            *(addr_ptr++) = FUNCTION;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            for (unsigned i = 0; i < fn.size(); ++i) {
                *(addr_ptr++) = fn[i];
            }
            *(addr_ptr++) = '\0';
            return addr_ptr;
        }

        byte* opfcall(byte* addr_ptr, int_op clsr, int_op ret) {
            /*  Inserts fcall instruction to bytecode.
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, FCALL, clsr, ret);
            return addr_ptr;
        }

        byte* opframe(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts frame instruction to bytecode.
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, FRAME, a, b);
            return addr_ptr;
        }

        byte* opparam(byte* addr_ptr, int_op a, int_op b) {
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

        byte* oppamv(byte* addr_ptr, int_op a, int_op b) {
            /*  Inserts pamv instruction to bytecode.
             *
             *  :params:
             *
             *  a - register number
             *  b - register number
             */
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, PAMV, a, b);
            return addr_ptr;
        }

        byte* opparef(byte* addr_ptr, int_op a, int_op b) {
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

        byte* oparg(byte* addr_ptr, int_op a, int_op b) {
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

        byte* opargc(byte* addr_ptr, int_op a) {
            /*  Inserts argc instruction to bytecode.
             *
             *  :params:
             *
             *  a - target register
             */
            *(addr_ptr++) = ARGC;
            addr_ptr = insertIntegerOperand(addr_ptr, a);
            return addr_ptr;
        }

        byte* opcall(byte* addr_ptr, int_op reg, const string& fn_name) {
            /*  Inserts call instruction.
             *  Byte offset is calculated automatically.
             */
            *(addr_ptr++) = CALL;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            for (unsigned i = 0; i < fn_name.size(); ++i) {
                *(addr_ptr++) = fn_name[i];
            }
            *(addr_ptr++) = '\0';
            return addr_ptr;
        }

        byte* opprocess(byte* addr_ptr, int_op reg, const string& fn_name) {
            *(addr_ptr++) = PROCESS;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            for (unsigned i = 0; i < fn_name.size(); ++i) {
                *(addr_ptr++) = fn_name[i];
            }
            *(addr_ptr++) = '\0';
            return addr_ptr;
        }

        byte* opthjoin(byte* addr_ptr, int_op target, int_op source) {
            *(addr_ptr++) = THJOIN;
            addr_ptr = insertIntegerOperand(addr_ptr, target);
            addr_ptr = insertIntegerOperand(addr_ptr, source);
            return addr_ptr;
        }

        byte* opthreceive(byte* addr_ptr, int_op reg) {
            *(addr_ptr++) = THRECEIVE;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* opwatchdog(byte* addr_ptr, const string& fn_name) {
            *(addr_ptr++) = WATCHDOG;
            for (unsigned i = 0; i < fn_name.size(); ++i) {
                *(addr_ptr++) = fn_name[i];
            }
            *(addr_ptr++) = '\0';
            return addr_ptr;
        }

        byte* opjump(byte* addr_ptr, uint64_t addr) {
            /*  Inserts jump instruction. Parameter is instruction index.
             *  Byte offset is calculated automatically.
             *
             *  :params:
             *
             *  addr:int    - index of the instruction to which to branch
             */
            *(addr_ptr++) = JUMP;

            // we *know* that this location in the byte array points to uint64_t so
            // the reinterpret_cast<> is justified
            *(reinterpret_cast<uint64_t*>(addr_ptr)) = addr;
            pointer::inc<uint64_t, byte>(addr_ptr);

            return addr_ptr;
        }

        byte* opbranch(byte* addr_ptr, int_op regc, uint64_t addr_truth, uint64_t addr_false) {
            /*  Inserts branch instruction.
             *  Byte offset is calculated automatically.
             */
            *(addr_ptr++) = BRANCH;
            addr_ptr = insertIntegerOperand(addr_ptr, regc);

            // we *know* that following locations in the byte array point to uint64_t so
            // the reinterpret_cast<> is justified
            *(reinterpret_cast<uint64_t*>(addr_ptr)) = addr_truth;
            pointer::inc<uint64_t, byte>(addr_ptr);
            *(reinterpret_cast<uint64_t*>(addr_ptr)) = addr_false;
            pointer::inc<uint64_t, byte>(addr_ptr);

            return addr_ptr;
        }

        byte* optry(byte* addr_ptr) {
            /*  Inserts try instruction.
             */
            *(addr_ptr++) = TRY;
            return addr_ptr;
        }

        byte* opcatch(byte* addr_ptr, const string& type_name, const string& block_name) {
            /*  Inserts catch instruction.
             */
            *(addr_ptr++) = CATCH;

            // the type
            for (unsigned i = 1; i < type_name.size()-1; ++i) {
                *(addr_ptr++) = type_name[i];
            }
            *(addr_ptr++) = '\0';

            // catcher block name
            for (unsigned i = 0; i < block_name.size(); ++i) {
                *(addr_ptr++) = block_name[i];
            }
            *(addr_ptr++) = '\0';

            return addr_ptr;
        }

        byte* oppull(byte* addr_ptr, int_op regno) {
            /*  Inserts throw instuction.
             */
            *(addr_ptr++) = PULL;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            return addr_ptr;
        }

        byte* openter(byte* addr_ptr, const string& block_name) {
            /*  Inserts enter instruction.
             *  Byte offset is calculated automatically.
             */
            *(addr_ptr++) = ENTER;
            for (unsigned i = 0; i < block_name.size(); ++i) {
                *(addr_ptr++) = block_name[i];
            }
            *(addr_ptr++) = '\0';
            return addr_ptr;
        }

        byte* opthrow(byte* addr_ptr, int_op regno) {
            /*  Inserts throw instuction.
             */
            *(addr_ptr++) = THROW;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            return addr_ptr;
        }

        byte* opleave(byte* addr_ptr) {
            /*  Inserts leave instruction.
             */
            *(addr_ptr++) = LEAVE;
            return addr_ptr;
        }

        byte* opimport(byte* addr_ptr, const string& module_name) {
            /*  Inserts eximport instruction.
             */
            *(addr_ptr++) = IMPORT;
            for (unsigned i = 1; i < module_name.size()-1; ++i) {
                *(addr_ptr++) = module_name[i];
            }
            *(addr_ptr++) = char(0);
            return addr_ptr;
        }

        byte* oplink(byte* addr_ptr, const string& module_name) {
            /*  Inserts eximport instruction.
             */
            *(addr_ptr++) = LINK;
            for (unsigned i = 0; i < module_name.size(); ++i) {
                *(addr_ptr++) = module_name[i];
            }
            *(addr_ptr++) = char(0);
            return addr_ptr;
        }

        byte* opclass(byte* addr_ptr, int_op reg, const string& class_name) {
            /*  Inserts class instuction.
             */
            *(addr_ptr++) = CLASS;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            for (unsigned i = 0; i < class_name.size(); ++i) {
                *(addr_ptr++) = class_name[i];
            }
            *(addr_ptr++) = '\0';
            return addr_ptr;
        }

        byte* opderive(byte* addr_ptr, int_op reg, const string& base_class_name) {
            /*  Inserts derive instuction.
             */
            *(addr_ptr++) = DERIVE;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            for (unsigned i = 0; i < base_class_name.size(); ++i) {
                *(addr_ptr++) = base_class_name[i];
            }
            *(addr_ptr++) = '\0';
            return addr_ptr;
        }

        byte* opattach(byte* addr_ptr, int_op reg, const string& function_name, const string& method_name) {
            /*  Inserts derive instuction.
             */
            *(addr_ptr++) = ATTACH;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            for (unsigned i = 0; i < function_name.size(); ++i) {
                *(addr_ptr++) = function_name[i];
            }
            *(addr_ptr++) = '\0';
            for (unsigned i = 0; i < method_name.size(); ++i) {
                *(addr_ptr++) = method_name[i];
            }
            *(addr_ptr++) = '\0';
            return addr_ptr;
        }

        byte* opregister(byte* addr_ptr, int_op regno) {
            /*  Inserts register instuction.
             */
            *(addr_ptr++) = REGISTER;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            return addr_ptr;
        }

        byte* opnew(byte* addr_ptr, int_op reg, const string& class_name) {
            /*  Inserts new instuction.
             */
            *(addr_ptr++) = NEW;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            for (unsigned i = 0; i < class_name.size(); ++i) {
                *(addr_ptr++) = class_name[i];
            }
            *(addr_ptr++) = '\0';
            return addr_ptr;
        }

        byte* opmsg(byte* addr_ptr, int_op reg, const string& method_name) {
            /*  Inserts msg instuction.
             */
            *(addr_ptr++) = MSG;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            for (unsigned i = 0; i < method_name.size(); ++i) {
                *(addr_ptr++) = method_name[i];
            }
            *(addr_ptr++) = '\0';
            return addr_ptr;
        }

        byte* opinsert(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, INSERT, rega, regb, regr);
            return addr_ptr;
        }

        byte* opremove(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, REMOVE, rega, regb, regr);
            return addr_ptr;
        }

        byte* opreturn(byte* addr_ptr) {
            *(addr_ptr++) = RETURN;
            return addr_ptr;
        }

        byte* ophalt(byte* addr_ptr) {
            /*  Inserts halt instruction.
             */
            *(addr_ptr++) = HALT;
            return addr_ptr;
        }
    }
}
