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


static byte* insert_ri_operand(byte* addr_ptr, int_op op) {
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
    *(reinterpret_cast<viua::internals::types::register_index*>(addr_ptr))  = op.value;
    pointer::inc<viua::internals::types::register_index, byte>(addr_ptr);

    return addr_ptr;
}

static byte* insert_two_ri_instruction(byte* addr_ptr, enum OPCODE instruction, int_op a, int_op b) {
    *(addr_ptr++) = instruction;
    addr_ptr = insert_ri_operand(addr_ptr, a);
    return insert_ri_operand(addr_ptr, b);
}

static byte* insert_three_ri_instruction(byte* addr_ptr, enum OPCODE instruction, int_op a, int_op b, int_op c) {
    *(addr_ptr++) = instruction;
    addr_ptr = insert_ri_operand(addr_ptr, a);
    addr_ptr = insert_ri_operand(addr_ptr, b);
    return insert_ri_operand(addr_ptr, c);
}

static byte* insert_two_ri_and_primitive_int_instruction(byte* addr_ptr, enum OPCODE instruction, int_op a, int_op b, int_op c) {
            addr_ptr = insert_two_ri_instruction(addr_ptr, instruction, a, b);

            // FIXME types different from OT_REGISTER_REFERENCE and OT_INT should
            // throw an error
            if (c.type == IntegerOperandType::REGISTER_REFERENCE) {
                *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_REGISTER_REFERENCE;
            } else {
                *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_INT;
            }
            pointer::inc<OperandType, byte>(addr_ptr);
            *(reinterpret_cast<int*>(addr_ptr))  = c.value;
            pointer::inc<int, byte>(addr_ptr);

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
            return insert_ri_operand(addr_ptr, regno);
        }

        byte* opistore(byte* addr_ptr, int_op regno, int_op i) {
            *(addr_ptr++) = ISTORE;
            addr_ptr = insert_ri_operand(addr_ptr, regno);

            *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_INT;
            pointer::inc<OperandType, byte>(addr_ptr);
            *(reinterpret_cast<int*>(addr_ptr))  = i.value;
            pointer::inc<int, byte>(addr_ptr);

            return addr_ptr;
        }

        byte* opiinc(byte* addr_ptr, int_op regno) {
            *(addr_ptr++) = IINC;
            return insert_ri_operand(addr_ptr, regno);
        }

        byte* opidec(byte* addr_ptr, int_op regno) {
            *(addr_ptr++) = IDEC;
            return insert_ri_operand(addr_ptr, regno);
        }

        byte* opfstore(byte* addr_ptr, int_op regno, float f) {
            *(addr_ptr++) = FSTORE;
            addr_ptr = insert_ri_operand(addr_ptr, regno);
            *(reinterpret_cast<float*>(addr_ptr))  = f;
            pointer::inc<float, byte>(addr_ptr);

            return addr_ptr;
        }

        byte* opitof(byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, ITOF, a, b);
        }

        byte* opftoi(byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, FTOI, a, b);
        }

        byte* opstoi(byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, STOI, a, b);
        }

        byte* opstof(byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, STOF, a, b);
        }

        static byte* encode_operand_type(byte* addr_ptr, string operand_type) {
            if (operand_type == "int") {
                *(addr_ptr++) = OperandType::OT_INT;
            } else if (operand_type == "int8") {
                *(addr_ptr++) = OperandType::OT_INT8;
            } else if (operand_type == "int16") {
                *(addr_ptr++) = OperandType::OT_INT16;
            } else if (operand_type == "int32") {
                *(addr_ptr++) = OperandType::OT_INT32;
            } else if (operand_type == "int64") {
                *(addr_ptr++) = OperandType::OT_INT64;
            } else if (operand_type == "uint") {
                *(addr_ptr++) = OperandType::OT_UINT;
            } else if (operand_type == "uint8") {
                *(addr_ptr++) = OperandType::OT_UINT8;
            } else if (operand_type == "uint16") {
                *(addr_ptr++) = OperandType::OT_UINT16;
            } else if (operand_type == "uint32") {
                *(addr_ptr++) = OperandType::OT_UINT32;
            } else if (operand_type == "uint64") {
                *(addr_ptr++) = OperandType::OT_UINT64;
            } else if (operand_type == "float") {
                *(addr_ptr++) = OperandType::OT_FLOAT;
            } else if (operand_type == "float32") {
                *(addr_ptr++) = OperandType::OT_FLOAT32;
            } else if (operand_type == "float64") {
                *(addr_ptr++) = OperandType::OT_FLOAT64;
            } else {
                *(addr_ptr++) = OperandType::OT_VOID;
            }
            return addr_ptr;
        }
        static byte* emit_instruction_alu(byte* addr_ptr, OPCODE instruction, std::string result_type, int_op target, int_op lhs, int_op rhs) {
            *(addr_ptr++) = instruction;

            addr_ptr = encode_operand_type(addr_ptr, result_type);
            addr_ptr = insert_ri_operand(addr_ptr, target);
            addr_ptr = insert_ri_operand(addr_ptr, lhs);
            return insert_ri_operand(addr_ptr, rhs);
        }
        byte* opadd(byte* addr_ptr, std::string result_type, int_op target, int_op lhs, int_op rhs) {
            return emit_instruction_alu(addr_ptr, ADD, result_type, target, lhs, rhs);
        }
        byte* opsub(byte* addr_ptr, std::string result_type, int_op target, int_op lhs, int_op rhs) {
            return emit_instruction_alu(addr_ptr, SUB, result_type, target, lhs, rhs);
        }
        byte* opmul(byte* addr_ptr, std::string result_type, int_op target, int_op lhs, int_op rhs) {
            return emit_instruction_alu(addr_ptr, MUL, result_type, target, lhs, rhs);
        }
        byte* opdiv(byte* addr_ptr, std::string result_type, int_op target, int_op lhs, int_op rhs) {
            return emit_instruction_alu(addr_ptr, DIV, result_type, target, lhs, rhs);
        }
        byte* oplt(byte* addr_ptr, std::string result_type, int_op target, int_op lhs, int_op rhs) {
            return emit_instruction_alu(addr_ptr, LT, result_type, target, lhs, rhs);
        }
        byte* oplte(byte* addr_ptr, std::string result_type, int_op target, int_op lhs, int_op rhs) {
            return emit_instruction_alu(addr_ptr, LTE, result_type, target, lhs, rhs);
        }
        byte* opgt(byte* addr_ptr, std::string result_type, int_op target, int_op lhs, int_op rhs) {
            return emit_instruction_alu(addr_ptr, GT, result_type, target, lhs, rhs);
        }
        byte* opgte(byte* addr_ptr, std::string result_type, int_op target, int_op lhs, int_op rhs) {
            return emit_instruction_alu(addr_ptr, GTE, result_type, target, lhs, rhs);
        }
        byte* opeq(byte* addr_ptr, std::string result_type, int_op target, int_op lhs, int_op rhs) {
            return emit_instruction_alu(addr_ptr, EQ, result_type, target, lhs, rhs);
        }

        byte* opstrstore(byte* addr_ptr, int_op reg, string s) {
            *(addr_ptr++) = STRSTORE;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insertString(addr_ptr, s.substr(1, s.size()-2));
        }

        byte* opvec(byte* addr_ptr, int_op index, int_op pack_start_index, int_op pack_length) {
            *(addr_ptr++) = VEC;
            addr_ptr = insert_ri_operand(addr_ptr, index);
            addr_ptr = insert_ri_operand(addr_ptr, pack_start_index);
            return insert_ri_operand(addr_ptr, pack_length);
        }

        byte* opvinsert(byte* addr_ptr, int_op vec, int_op src, int_op dst) {
            return insert_three_ri_instruction(addr_ptr, VINSERT, vec, src, dst);
        }

        byte* opvpush(byte* addr_ptr, int_op vec, int_op src) {
            return insert_two_ri_instruction(addr_ptr, VPUSH, vec, src);
        }

        byte* opvpop(byte* addr_ptr, int_op vec, int_op dst, int_op pos) {
            return insert_two_ri_and_primitive_int_instruction(addr_ptr, VPOP, vec, dst, pos);
        }

        byte* opvat(byte* addr_ptr, int_op vec, int_op dst, int_op at) {
            return insert_two_ri_and_primitive_int_instruction(addr_ptr, VAT, vec, dst, at);
        }

        byte* opvlen(byte* addr_ptr, int_op vec, int_op reg) {
            return insert_two_ri_instruction(addr_ptr, VLEN, vec, reg);
        }

        byte* opnot(byte* addr_ptr, int_op target, int_op source) {
            *(addr_ptr++) = NOT;
            addr_ptr = insert_ri_operand(addr_ptr, target);
            return insert_ri_operand(addr_ptr, source);
        }

        byte* opand(byte* addr_ptr, int_op regr, int_op rega, int_op regb) {
            return insert_three_ri_instruction(addr_ptr, AND, regr, rega, regb);
        }

        byte* opor(byte* addr_ptr, int_op regr, int_op rega, int_op regb) {
            return insert_three_ri_instruction(addr_ptr, OR, regr, rega, regb);
        }

        byte* opmove(byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, MOVE, a, b);
        }

        byte* opcopy(byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, COPY, a, b);
        }

        byte* opptr(byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, PTR, a, b);
        }

        byte* opswap(byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, SWAP, a, b);
        }

        byte* opdelete(byte* addr_ptr, int_op reg) {
            *(addr_ptr++) = DELETE;
            return insert_ri_operand(addr_ptr, reg);
        }

        byte* opisnull(byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, ISNULL, a, b);
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
            return insert_ri_operand(addr_ptr, reg);
        }

        byte* optmpro(byte* addr_ptr, int_op reg) {
            *(addr_ptr++) = TMPRO;
            return insert_ri_operand(addr_ptr, reg);
        }

        byte* opprint(byte* addr_ptr, int_op reg) {
            *(addr_ptr++) = PRINT;
            return insert_ri_operand(addr_ptr, reg);
        }

        byte* opecho(byte* addr_ptr, int_op reg) {
            *(addr_ptr++) = ECHO;
            return insert_ri_operand(addr_ptr, reg);
        }

        byte* opcapture(byte* addr_ptr, int_op target_closure, int_op target_register, int_op source_register) {
            *(addr_ptr++) = CAPTURE;
            addr_ptr = insert_ri_operand(addr_ptr, target_closure);
            addr_ptr = insert_ri_operand(addr_ptr, target_register);
            return insert_ri_operand(addr_ptr, source_register);
        }

        byte* opcapturecopy(byte* addr_ptr, int_op target_closure, int_op target_register, int_op source_register) {
            *(addr_ptr++) = CAPTURECOPY;
            addr_ptr = insert_ri_operand(addr_ptr, target_closure);
            addr_ptr = insert_ri_operand(addr_ptr, target_register);
            return insert_ri_operand(addr_ptr, source_register);
        }

        byte* opcapturemove(byte* addr_ptr, int_op target_closure, int_op target_register, int_op source_register) {
            *(addr_ptr++) = CAPTUREMOVE;
            addr_ptr = insert_ri_operand(addr_ptr, target_closure);
            addr_ptr = insert_ri_operand(addr_ptr, target_register);
            return insert_ri_operand(addr_ptr, source_register);
        }

        byte* opclosure(byte* addr_ptr, int_op reg, const string& fn) {
            *(addr_ptr++) = CLOSURE;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insertString(addr_ptr, fn);
        }

        byte* opfunction(byte* addr_ptr, int_op reg, const string& fn) {
            *(addr_ptr++) = FUNCTION;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insertString(addr_ptr, fn);
        }

        byte* opfcall(byte* addr_ptr, int_op clsr, int_op ret) {
            return insert_two_ri_instruction(addr_ptr, FCALL, clsr, ret);
        }

        byte* opframe(byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, FRAME, a, b);
        }

        byte* opparam(byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, PARAM, a, b);
        }

        byte* oppamv(byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, PAMV, a, b);
        }

        byte* oparg(byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, ARG, a, b);
        }

        byte* opargc(byte* addr_ptr, int_op a) {
            *(addr_ptr++) = ARGC;
            return insert_ri_operand(addr_ptr, a);
        }

        byte* opcall(byte* addr_ptr, int_op reg, const string& fn_name) {
            *(addr_ptr++) = CALL;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insertString(addr_ptr, fn_name);
        }

        byte* optailcall(byte* addr_ptr, const string& fn_name) {
            *(addr_ptr++) = TAILCALL;
            return insertString(addr_ptr, fn_name);
        }

        byte* opprocess(byte* addr_ptr, int_op reg, const string& fn_name) {
            *(addr_ptr++) = PROCESS;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insertString(addr_ptr, fn_name);
        }

        byte* opself(byte* addr_ptr, int_op target) {
            *(addr_ptr++) = SELF;
            return insert_ri_operand(addr_ptr, target);
        }

        byte* opjoin(byte* addr_ptr, int_op target, int_op source, int_op timeout) {
            *(addr_ptr++) = JOIN;
            addr_ptr = insert_ri_operand(addr_ptr, target);
            addr_ptr = insert_ri_operand(addr_ptr, source);
            return insert_ri_operand(addr_ptr, timeout);
        }

        byte* opsend(byte* addr_ptr, int_op target, int_op source) {
            *(addr_ptr++) = SEND;
            addr_ptr = insert_ri_operand(addr_ptr, target);
            return insert_ri_operand(addr_ptr, source);
        }

        byte* opreceive(byte* addr_ptr, int_op reg, int_op timeout) {
            *(addr_ptr++) = RECEIVE;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insert_ri_operand(addr_ptr, timeout);
        }

        byte* opwatchdog(byte* addr_ptr, const string& fn_name) {
            *(addr_ptr++) = WATCHDOG;
            return insertString(addr_ptr, fn_name);
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
            addr_ptr = insert_ri_operand(addr_ptr, regc);

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

        byte* opdraw(byte* addr_ptr, int_op regno) {
            *(addr_ptr++) = DRAW;
            return insert_ri_operand(addr_ptr, regno);
        }

        byte* openter(byte* addr_ptr, const string& block_name) {
            *(addr_ptr++) = ENTER;
            return insertString(addr_ptr, block_name);
        }

        byte* opthrow(byte* addr_ptr, int_op regno) {
            *(addr_ptr++) = THROW;
            return insert_ri_operand(addr_ptr, regno);
        }

        byte* opleave(byte* addr_ptr) {
            *(addr_ptr++) = LEAVE;
            return addr_ptr;
        }

        byte* opimport(byte* addr_ptr, const string& module_name) {
            *(addr_ptr++) = IMPORT;
            return insertString(addr_ptr, module_name.substr(1, module_name.size()-2));
        }

        byte* oplink(byte* addr_ptr, const string& module_name) {
            *(addr_ptr++) = LINK;
            return insertString(addr_ptr, module_name);
        }

        byte* opclass(byte* addr_ptr, int_op reg, const string& class_name) {
            *(addr_ptr++) = CLASS;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insertString(addr_ptr, class_name);
        }

        byte* opderive(byte* addr_ptr, int_op reg, const string& base_class_name) {
            *(addr_ptr++) = DERIVE;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insertString(addr_ptr, base_class_name);
        }

        byte* opattach(byte* addr_ptr, int_op reg, const string& function_name, const string& method_name) {
            *(addr_ptr++) = ATTACH;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            addr_ptr = insertString(addr_ptr, function_name);
            return insertString(addr_ptr, method_name);
        }

        byte* opregister(byte* addr_ptr, int_op regno) {
            *(addr_ptr++) = REGISTER;
            return insert_ri_operand(addr_ptr, regno);
        }

        byte* opnew(byte* addr_ptr, int_op reg, const string& class_name) {
            *(addr_ptr++) = NEW;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insertString(addr_ptr, class_name);
        }

        byte* opmsg(byte* addr_ptr, int_op reg, const string& method_name) {
            *(addr_ptr++) = MSG;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insertString(addr_ptr, method_name);
        }

        byte* opinsert(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            return insert_three_ri_instruction(addr_ptr, INSERT, rega, regb, regr);
        }

        byte* opremove(byte* addr_ptr, int_op rega, int_op regb, int_op regr) {
            return insert_three_ri_instruction(addr_ptr, REMOVE, rega, regb, regr);
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
