/*
 *  Copyright (C) 2015, 2016 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <viua/cg/bytecode/instructions.h>
#include <viua/bytecode/operand_types.h>
using namespace std;


int_op::int_op(): type(IntegerOperandType::PLAIN), value(0) {
}
int_op::int_op(IntegerOperandType t, int n): type(t), value(n) {
}
int_op::int_op(int n): type(IntegerOperandType::PLAIN), value(n) {
}


static byte* insertIntegerOperand(byte* addr_ptr, int_op op) {
    /** Insert integer operand into bytecode.
     *
     *  When using integer operand, it usually is a plain number - which translates to a regsiter index.
     *  However, when preceded by `@` integer operand will not be interpreted directly, but instead viua::kernel::Kernel
     *  will look into a register the integer points to, fetch an integer from this register and
     *  use the fetched register as the operand.
     */
    if (op.type == IntegerOperandType::VOID) {
        *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_VOID;
        pointer::inc<OperandType, byte>(addr_ptr);
        return addr_ptr;
    }

    /* NOTICE: reinterpret_cast<>'s are ugly, but we know what we're doing here.
     * Since we store everything in a big array of bytes we have to cast incompatible pointers to
     * actually put *valid* data inside it.
     */
    if (op.type == IntegerOperandType::POINTER_DEREFERENCE) {
        *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_POINTER;
    } else if (op.type == IntegerOperandType::REGISTER_REFERENCE) {
        *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_REGISTER_REFERENCE;
    } else {
        *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_REGISTER_INDEX;
    }
    pointer::inc<OperandType, byte>(addr_ptr);
    *(reinterpret_cast<int*>(addr_ptr))  = op.value;
    pointer::inc<decltype(op.value), byte>(addr_ptr);

    return addr_ptr;
}

static byte* insertTwoIntegerOpsInstruction(byte* addr_ptr, enum OPCODE instruction, int_op a, int_op b) {
    *(addr_ptr++) = instruction;
    addr_ptr = insertIntegerOperand(addr_ptr, a);
    addr_ptr = insertIntegerOperand(addr_ptr, b);
    return addr_ptr;
}

static byte* insertThreeIntegerOpsInstruction(byte* addr_ptr, enum OPCODE instruction, int_op a, int_op b, int_op c) {
    *(addr_ptr++) = instruction;
    addr_ptr = insertIntegerOperand(addr_ptr, a);
    addr_ptr = insertIntegerOperand(addr_ptr, b);
    addr_ptr = insertIntegerOperand(addr_ptr, c);
    return addr_ptr;
}

static byte* insertString(byte* ptr, const string& s) {
    for (unsigned i = 0; i < s.size(); ++i) {
        *(ptr++) = static_cast<unsigned char>(s[i]);
    }
    *(ptr++) = '\0';
    return ptr;
}

namespace cg {
    namespace bytecode {
        byte* opnop(byte* addr_ptr) {
            *(addr_ptr++) = NOP;
            return addr_ptr;
        }

        byte* opizero(byte* addr_ptr, int_op regno) {
            *(addr_ptr++) = IZERO;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            return addr_ptr;
        }

        byte* opistore(byte* addr_ptr, int_op regno, int_op i) {
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, ISTORE, regno, i);
            return addr_ptr;
        }

        byte* opiadd(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, IADD, rega, regb, regr);
            return addr_ptr;
        }

        byte* opisub(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, ISUB, rega, regb, regr);
            return addr_ptr;
        }

        byte* opimul(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, IMUL, rega, regb, regr);
            return addr_ptr;
        }

        byte* opidiv(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, IDIV, rega, regb, regr);
            return addr_ptr;
        }

        byte* opiinc(byte* addr_ptr, int_op regno) {
            *(addr_ptr++) = IINC;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            return addr_ptr;
        }

        byte* opidec(byte* addr_ptr, int_op regno) {
            *(addr_ptr++) = IDEC;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            return addr_ptr;
        }

        byte* opilt(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, ILT, rega, regb, regr);
            return addr_ptr;
        }

        byte* opilte(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, ILTE, rega, regb, regr);
            return addr_ptr;
        }

        byte* opigt(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, IGT, rega, regb, regr);
            return addr_ptr;
        }

        byte* opigte(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, IGTE, rega, regb, regr);
            return addr_ptr;
        }

        byte* opieq(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, IEQ, rega, regb, regr);
            return addr_ptr;
        }

        byte* opfstore(byte* addr_ptr, int_op regno, float f) {
            *(addr_ptr++) = FSTORE;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            *(reinterpret_cast<float*>(addr_ptr))  = f;
            pointer::inc<float, byte>(addr_ptr);

            return addr_ptr;
        }

        byte* opfadd(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FADD, rega, regb, regr);
            return addr_ptr;
        }

        byte* opfsub(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FSUB, rega, regb, regr);
            return addr_ptr;
        }

        byte* opfmul(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FMUL, rega, regb, regr);
            return addr_ptr;
        }

        byte* opfdiv(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FDIV, rega, regb, regr);
            return addr_ptr;
        }

        byte* opflt(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FLT, rega, regb, regr);
            return addr_ptr;
        }

        byte* opflte(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FLTE, rega, regb, regr);
            return addr_ptr;
        }

        byte* opfgt(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FGT, rega, regb, regr);
            return addr_ptr;
        }

        byte* opfgte(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FGTE, rega, regb, regr);
            return addr_ptr;
        }

        byte* opfeq(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FEQ, rega, regb, regr);
            return addr_ptr;
        }

        byte* opitof(byte* addr_ptr, int_op a, int_op b) {
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, ITOF, a, b);
            return addr_ptr;
        }

        byte* opftoi(byte* addr_ptr, int_op a, int_op b) {
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, FTOI, a, b);
            return addr_ptr;
        }

        byte* opstoi(byte* addr_ptr, int_op a, int_op b) {
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, STOI, a, b);
            return addr_ptr;
        }

        byte* opstof(byte* addr_ptr, int_op a, int_op b) {
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, STOF, a, b);
            return addr_ptr;
        }

        byte* opstrstore(byte* addr_ptr, int_op reg, string s) {
            *(addr_ptr++) = STRSTORE;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            addr_ptr = insertString(addr_ptr, s.substr(1, s.size()-2));
            return addr_ptr;
        }

        byte* opvec(byte* addr_ptr, int_op index, int_op pack_start_index, int_op pack_length) {
            *(addr_ptr++) = VEC;
            addr_ptr = insertIntegerOperand(addr_ptr, index);
            addr_ptr = insertIntegerOperand(addr_ptr, pack_start_index);
            addr_ptr = insertIntegerOperand(addr_ptr, pack_length);
            return addr_ptr;
        }

        byte* opvinsert(byte* addr_ptr, int_op vec, int_op src, int_op dst) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, VINSERT, vec, src, dst);
            return addr_ptr;
        }

        byte* opvpush(byte* addr_ptr, int_op vec, int_op src) {
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, VPUSH, vec, src);
            return addr_ptr;
        }

        byte* opvpop(byte* addr_ptr, int_op vec, int_op dst, int_op pos) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, VPOP, vec, dst, pos);
            return addr_ptr;
        }

        byte* opvat(byte* addr_ptr, int_op vec, int_op dst, int_op at) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, VAT, vec, dst, at);
            return addr_ptr;
        }

        byte* opvlen(byte* addr_ptr, int_op vec, int_op reg) {
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, VLEN, vec, reg);
            return addr_ptr;
        }

        byte* opnot(byte* addr_ptr, int_op target, int_op source) {
            *(addr_ptr++) = NOT;
            addr_ptr = insertIntegerOperand(addr_ptr, target);
            addr_ptr = insertIntegerOperand(addr_ptr, source);
            return addr_ptr;
        }

        byte* opand(byte* addr_ptr, int_op regr, int_op rega, int_op regb) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, AND, regr, rega, regb);
            return addr_ptr;
        }

        byte* opor(byte* addr_ptr, int_op regr, int_op rega, int_op regb) {
            addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, OR, regr, rega, regb);
            return addr_ptr;
        }

        byte* opmove(byte* addr_ptr, int_op a, int_op b) {
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, MOVE, a, b);
            return addr_ptr;
        }

        byte* opcopy(byte* addr_ptr, int_op a, int_op b) {
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, COPY, a, b);
            return addr_ptr;
        }

        byte* opptr(byte* addr_ptr, int_op a, int_op b) {
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, PTR, a, b);
            return addr_ptr;
        }

        byte* opswap(byte* addr_ptr, int_op a, int_op b) {
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, SWAP, a, b);
            return addr_ptr;
        }

        byte* opdelete(byte* addr_ptr, int_op reg) {
            *(addr_ptr++) = DELETE;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* opisnull(byte* addr_ptr, int_op a, int_op b) {
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, ISNULL, a, b);
            return addr_ptr;
        }

        byte* opress(byte* addr_ptr, const string& a) {
            *(addr_ptr++) = RESS;
            int register_set_marker = 0;
            if (a == "global") {
                register_set_marker = 0;
            } else if (a == "local") {
                register_set_marker = 1;
            } else if (a == "static") {
                register_set_marker = 2;
            } else if (a == "temp") {
                register_set_marker = 3;
            } else {
                // FIXME: detect invalid register set names
                // after switching to token-based code generation
                // it will not be necessary
            }
            *(reinterpret_cast<int*>(addr_ptr)) = register_set_marker;
            pointer::inc<int, byte>(addr_ptr);
            return addr_ptr;
        }

        byte* optmpri(byte* addr_ptr, int_op reg) {
            *(addr_ptr++) = TMPRI;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* optmpro(byte* addr_ptr, int_op reg) {
            *(addr_ptr++) = TMPRO;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* opprint(byte* addr_ptr, int_op reg) {
            *(addr_ptr++) = PRINT;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* opecho(byte* addr_ptr, int_op reg) {
            *(addr_ptr++) = ECHO;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            return addr_ptr;
        }

        byte* opcapture(byte* addr_ptr, int_op target_closure, int_op target_register, int_op source_register) {
            *(addr_ptr++) = CAPTURE;
            addr_ptr = insertIntegerOperand(addr_ptr, target_closure);
            addr_ptr = insertIntegerOperand(addr_ptr, target_register);
            addr_ptr = insertIntegerOperand(addr_ptr, source_register);
            return addr_ptr;
        }

        byte* opcapturecopy(byte* addr_ptr, int_op target_closure, int_op target_register, int_op source_register) {
            *(addr_ptr++) = CAPTURECOPY;
            addr_ptr = insertIntegerOperand(addr_ptr, target_closure);
            addr_ptr = insertIntegerOperand(addr_ptr, target_register);
            addr_ptr = insertIntegerOperand(addr_ptr, source_register);
            return addr_ptr;
        }

        byte* opcapturemove(byte* addr_ptr, int_op target_closure, int_op target_register, int_op source_register) {
            *(addr_ptr++) = CAPTUREMOVE;
            addr_ptr = insertIntegerOperand(addr_ptr, target_closure);
            addr_ptr = insertIntegerOperand(addr_ptr, target_register);
            addr_ptr = insertIntegerOperand(addr_ptr, source_register);
            return addr_ptr;
        }

        byte* opclosure(byte* addr_ptr, int_op reg, const string& fn) {
            *(addr_ptr++) = CLOSURE;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            addr_ptr = insertString(addr_ptr, fn);
            return addr_ptr;
        }

        byte* opfunction(byte* addr_ptr, int_op reg, const string& fn) {
            *(addr_ptr++) = FUNCTION;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            addr_ptr = insertString(addr_ptr, fn);
            return addr_ptr;
        }

        byte* opfcall(byte* addr_ptr, int_op clsr, int_op ret) {
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, FCALL, clsr, ret);
            return addr_ptr;
        }

        byte* opframe(byte* addr_ptr, int_op a, int_op b) {
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, FRAME, a, b);
            return addr_ptr;
        }

        byte* opparam(byte* addr_ptr, int_op a, int_op b) {
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, PARAM, a, b);
            return addr_ptr;
        }

        byte* oppamv(byte* addr_ptr, int_op a, int_op b) {
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, PAMV, a, b);
            return addr_ptr;
        }

        byte* oparg(byte* addr_ptr, int_op a, int_op b) {
            addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, ARG, a, b);
            return addr_ptr;
        }

        byte* opargc(byte* addr_ptr, int_op a) {
            *(addr_ptr++) = ARGC;
            addr_ptr = insertIntegerOperand(addr_ptr, a);
            return addr_ptr;
        }

        byte* opcall(byte* addr_ptr, int_op reg, const string& fn_name) {
            *(addr_ptr++) = CALL;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            addr_ptr = insertString(addr_ptr, fn_name);
            return addr_ptr;
        }

        byte* optailcall(byte* addr_ptr, const string& fn_name) {
            *(addr_ptr++) = TAILCALL;
            addr_ptr = insertString(addr_ptr, fn_name);
            return addr_ptr;
        }

        byte* opprocess(byte* addr_ptr, int_op reg, const string& fn_name) {
            *(addr_ptr++) = PROCESS;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            addr_ptr = insertString(addr_ptr, fn_name);
            return addr_ptr;
        }

        byte* opself(byte* addr_ptr, int_op target) {
            *(addr_ptr++) = SELF;
            addr_ptr = insertIntegerOperand(addr_ptr, target);
            return addr_ptr;
        }

        byte* opjoin(byte* addr_ptr, int_op target, int_op source, int_op timeout) {
            *(addr_ptr++) = JOIN;
            addr_ptr = insertIntegerOperand(addr_ptr, target);
            addr_ptr = insertIntegerOperand(addr_ptr, source);
            addr_ptr = insertIntegerOperand(addr_ptr, timeout);
            return addr_ptr;
        }

        byte* opsend(byte* addr_ptr, int_op target, int_op source) {
            *(addr_ptr++) = SEND;
            addr_ptr = insertIntegerOperand(addr_ptr, target);
            addr_ptr = insertIntegerOperand(addr_ptr, source);
            return addr_ptr;
        }

        byte* opreceive(byte* addr_ptr, int_op reg, int_op timeout) {
            *(addr_ptr++) = RECEIVE;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            addr_ptr = insertIntegerOperand(addr_ptr, timeout);
            return addr_ptr;
        }

        byte* opwatchdog(byte* addr_ptr, const string& fn_name) {
            *(addr_ptr++) = WATCHDOG;
            addr_ptr = insertString(addr_ptr, fn_name);
            return addr_ptr;
        }

        byte* opjump(byte* addr_ptr, uint64_t addr) {
            *(addr_ptr++) = JUMP;

            // we *know* that this location in the byte array points to uint64_t so
            // the reinterpret_cast<> is justified
            *(reinterpret_cast<uint64_t*>(addr_ptr)) = addr;
            pointer::inc<uint64_t, byte>(addr_ptr);

            return addr_ptr;
        }

        byte* opif(byte* addr_ptr, int_op regc, uint64_t addr_truth, uint64_t addr_false) {
            *(addr_ptr++) = IF;
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
            *(addr_ptr++) = TRY;
            return addr_ptr;
        }

        byte* opcatch(byte* addr_ptr, const string& type_name, const string& block_name) {
            *(addr_ptr++) = CATCH;

            // the type
            addr_ptr = insertString(addr_ptr, type_name.substr(1, type_name.size()-2));

            // catcher block name
            addr_ptr = insertString(addr_ptr, block_name);

            return addr_ptr;
        }

        byte* oppull(byte* addr_ptr, int_op regno) {
            *(addr_ptr++) = PULL;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            return addr_ptr;
        }

        byte* openter(byte* addr_ptr, const string& block_name) {
            *(addr_ptr++) = ENTER;
            addr_ptr = insertString(addr_ptr, block_name);
            return addr_ptr;
        }

        byte* opthrow(byte* addr_ptr, int_op regno) {
            *(addr_ptr++) = THROW;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            return addr_ptr;
        }

        byte* opleave(byte* addr_ptr) {
            *(addr_ptr++) = LEAVE;
            return addr_ptr;
        }

        byte* opimport(byte* addr_ptr, const string& module_name) {
            *(addr_ptr++) = IMPORT;
            addr_ptr = insertString(addr_ptr, module_name.substr(1, module_name.size()-2));
            return addr_ptr;
        }

        byte* oplink(byte* addr_ptr, const string& module_name) {
            *(addr_ptr++) = LINK;
            addr_ptr = insertString(addr_ptr, module_name);
            return addr_ptr;
        }

        byte* opclass(byte* addr_ptr, int_op reg, const string& class_name) {
            *(addr_ptr++) = CLASS;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            addr_ptr = insertString(addr_ptr, class_name);
            return addr_ptr;
        }

        byte* opderive(byte* addr_ptr, int_op reg, const string& base_class_name) {
            *(addr_ptr++) = DERIVE;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            addr_ptr = insertString(addr_ptr, base_class_name);
            return addr_ptr;
        }

        byte* opattach(byte* addr_ptr, int_op reg, const string& function_name, const string& method_name) {
            *(addr_ptr++) = ATTACH;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            addr_ptr = insertString(addr_ptr, function_name);
            addr_ptr = insertString(addr_ptr, method_name);
            return addr_ptr;
        }

        byte* opregister(byte* addr_ptr, int_op regno) {
            *(addr_ptr++) = REGISTER;
            addr_ptr = insertIntegerOperand(addr_ptr, regno);
            return addr_ptr;
        }

        byte* opnew(byte* addr_ptr, int_op reg, const string& class_name) {
            *(addr_ptr++) = NEW;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            addr_ptr = insertString(addr_ptr, class_name);
            return addr_ptr;
        }

        byte* opmsg(byte* addr_ptr, int_op reg, const string& method_name) {
            *(addr_ptr++) = MSG;
            addr_ptr = insertIntegerOperand(addr_ptr, reg);
            addr_ptr = insertString(addr_ptr, method_name);
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
            *(addr_ptr++) = HALT;
            return addr_ptr;
        }
    }
}
