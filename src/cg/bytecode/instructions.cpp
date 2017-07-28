/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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

#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/operand_types.h>
#include <viua/cg/bytecode/instructions.h>
#include <viua/util/memory.h>
using namespace std;

using viua::util::memory::aligned_write;


int_op::int_op()
    : type(IntegerOperandType::PLAIN), rs_type(viua::internals::RegisterSets::CURRENT), value(0) {}
int_op::int_op(IntegerOperandType t, viua::internals::types::plain_int n)
    : type(t), rs_type(viua::internals::RegisterSets::CURRENT), value(n) {}
int_op::int_op(IntegerOperandType t, viua::internals::RegisterSets rst, viua::internals::types::plain_int n)
    : type(t), rs_type(rst), value(n) {}
int_op::int_op(viua::internals::types::plain_int n)
    : type(IntegerOperandType::PLAIN), rs_type(viua::internals::RegisterSets::CURRENT), value(n) {}

timeout_op::timeout_op() : type(IntegerOperandType::PLAIN), value(0) {}
timeout_op::timeout_op(IntegerOperandType t, viua::internals::types::timeout n) : type(t), value(n) {}
timeout_op::timeout_op(viua::internals::types::timeout n) : type(IntegerOperandType::PLAIN), value(n) {}


static viua::internals::types::byte* insert_ri_operand(viua::internals::types::byte* addr_ptr, int_op op) {
    /** Insert integer operand into bytecode.
     *
     *  When using integer operand, it usually is a plain number - which translates to a regsiter index.
     *  However, when preceded by `@` integer operand will not be interpreted directly, but instead
     * viua::kernel::Kernel
     *  will look into a register the integer points to, fetch an integer from this register and
     *  use the fetched register as the operand.
     */
    if (op.type == IntegerOperandType::VOID) {
        *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_VOID;
        pointer::inc<OperandType, viua::internals::types::byte>(addr_ptr);
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
    pointer::inc<OperandType, viua::internals::types::byte>(addr_ptr);

    aligned_write(addr_ptr) = op.value;
    pointer::inc<viua::internals::types::register_index, viua::internals::types::byte>(addr_ptr);

    *(reinterpret_cast<viua::internals::RegisterSets*>(addr_ptr)) = op.rs_type;
    pointer::inc<viua::internals::RegisterSets, viua::internals::types::byte>(addr_ptr);

    return addr_ptr;
}

static viua::internals::types::byte* insert_bool_operand(viua::internals::types::byte* addr_ptr, bool op) {
    if (op) {
        *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_TRUE;
    } else {
        *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_FALSE;
    }
    pointer::inc<OperandType, viua::internals::types::byte>(addr_ptr);

    return addr_ptr;
}

static viua::internals::types::byte* insert_two_ri_instruction(viua::internals::types::byte* addr_ptr,
                                                               enum OPCODE instruction, int_op a, int_op b) {
    *(addr_ptr++) = instruction;
    addr_ptr = insert_ri_operand(addr_ptr, a);
    return insert_ri_operand(addr_ptr, b);
}

static viua::internals::types::byte* insert_three_ri_instruction(viua::internals::types::byte* addr_ptr,
                                                                 enum OPCODE instruction, int_op a, int_op b,
                                                                 int_op c) {
    *(addr_ptr++) = instruction;
    addr_ptr = insert_ri_operand(addr_ptr, a);
    addr_ptr = insert_ri_operand(addr_ptr, b);
    return insert_ri_operand(addr_ptr, c);
}

static viua::internals::types::byte* insert_four_ri_instruction(viua::internals::types::byte* addr_ptr,
                                                                enum OPCODE instruction, int_op a, int_op b,
                                                                int_op c, int_op d) {
    *(addr_ptr++) = instruction;
    addr_ptr = insert_ri_operand(addr_ptr, a);
    addr_ptr = insert_ri_operand(addr_ptr, b);
    addr_ptr = insert_ri_operand(addr_ptr, c);
    return insert_ri_operand(addr_ptr, d);
}

static viua::internals::types::byte* insert_type_prefixed_string(viua::internals::types::byte* ptr,
                                                                 const string& s, const OperandType op_type) {
    *(reinterpret_cast<OperandType*>(ptr)) = op_type;
    pointer::inc<OperandType, viua::internals::types::byte>(ptr);
    for (std::string::size_type i = 0; i < s.size(); ++i) {
        *(ptr++) = static_cast<viua::internals::types::byte>(s[i]);
    }
    *(ptr++) = '\0';
    return ptr;
}

static auto insert_size_and_type_prefixed_bitstring(viua::internals::types::byte* ptr,
                                                    const vector<uint8_t> bit_string)
    -> viua::internals::types::byte* {
    *(reinterpret_cast<OperandType*>(ptr)) = OT_BITS;
    pointer::inc<OperandType, viua::internals::types::byte>(ptr);
    aligned_write(ptr) = bit_string.size();
    pointer::inc<viua::internals::types::bits_size, viua::internals::types::byte>(ptr);

    /*
     * Binaries (and other multibyte values) are stored in the Big Endian encoding.
     * Remember about this!
     */
    for (auto i = bit_string.size(); i; --i) {
        *ptr = bit_string.at(i - 1);
        ++ptr;
    }

    return ptr;
}

static viua::internals::types::byte* insertString(viua::internals::types::byte* ptr, const string& s) {
    for (std::string::size_type i = 0; i < s.size(); ++i) {
        *(ptr++) = static_cast<viua::internals::types::byte>(s[i]);
    }
    *(ptr++) = '\0';
    return ptr;
}

namespace cg {
    namespace bytecode {
        viua::internals::types::byte* opnop(viua::internals::types::byte* addr_ptr) {
            *(addr_ptr++) = NOP;
            return addr_ptr;
        }

        viua::internals::types::byte* opizero(viua::internals::types::byte* addr_ptr, int_op regno) {
            *(addr_ptr++) = IZERO;
            return insert_ri_operand(addr_ptr, regno);
        }

        viua::internals::types::byte* opistore(viua::internals::types::byte* addr_ptr, int_op regno,
                                               int_op i) {
            *(addr_ptr++) = ISTORE;
            addr_ptr = insert_ri_operand(addr_ptr, regno);

            *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_INT;
            pointer::inc<OperandType, viua::internals::types::byte>(addr_ptr);
            aligned_write(addr_ptr) = i.value;
            pointer::inc<viua::internals::types::plain_int, viua::internals::types::byte>(addr_ptr);

            return addr_ptr;
        }

        viua::internals::types::byte* opiinc(viua::internals::types::byte* addr_ptr, int_op regno) {
            *(addr_ptr++) = IINC;
            return insert_ri_operand(addr_ptr, regno);
        }

        viua::internals::types::byte* opidec(viua::internals::types::byte* addr_ptr, int_op regno) {
            *(addr_ptr++) = IDEC;
            return insert_ri_operand(addr_ptr, regno);
        }

        viua::internals::types::byte* opfstore(viua::internals::types::byte* addr_ptr, int_op regno,
                                               viua::internals::types::plain_float f) {
            *(addr_ptr++) = FSTORE;
            addr_ptr = insert_ri_operand(addr_ptr, regno);
            aligned_write(addr_ptr) = f;
            pointer::inc<viua::internals::types::plain_float, viua::internals::types::byte>(addr_ptr);

            return addr_ptr;
        }

        viua::internals::types::byte* opitof(viua::internals::types::byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, ITOF, a, b);
        }

        viua::internals::types::byte* opftoi(viua::internals::types::byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, FTOI, a, b);
        }

        viua::internals::types::byte* opstoi(viua::internals::types::byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, STOI, a, b);
        }

        viua::internals::types::byte* opstof(viua::internals::types::byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, STOF, a, b);
        }

        static viua::internals::types::byte* emit_instruction_alu(viua::internals::types::byte* addr_ptr,
                                                                  OPCODE instruction, int_op target,
                                                                  int_op lhs, int_op rhs) {
            return insert_three_ri_instruction(addr_ptr, instruction, target, lhs, rhs);
        }
        viua::internals::types::byte* opadd(viua::internals::types::byte* addr_ptr, int_op target, int_op lhs,
                                            int_op rhs) {
            return emit_instruction_alu(addr_ptr, ADD, target, lhs, rhs);
        }
        viua::internals::types::byte* opsub(viua::internals::types::byte* addr_ptr, int_op target, int_op lhs,
                                            int_op rhs) {
            return emit_instruction_alu(addr_ptr, SUB, target, lhs, rhs);
        }
        viua::internals::types::byte* opmul(viua::internals::types::byte* addr_ptr, int_op target, int_op lhs,
                                            int_op rhs) {
            return emit_instruction_alu(addr_ptr, MUL, target, lhs, rhs);
        }
        viua::internals::types::byte* opdiv(viua::internals::types::byte* addr_ptr, int_op target, int_op lhs,
                                            int_op rhs) {
            return emit_instruction_alu(addr_ptr, DIV, target, lhs, rhs);
        }
        viua::internals::types::byte* oplt(viua::internals::types::byte* addr_ptr, int_op target, int_op lhs,
                                           int_op rhs) {
            return emit_instruction_alu(addr_ptr, LT, target, lhs, rhs);
        }
        viua::internals::types::byte* oplte(viua::internals::types::byte* addr_ptr, int_op target, int_op lhs,
                                            int_op rhs) {
            return emit_instruction_alu(addr_ptr, LTE, target, lhs, rhs);
        }
        viua::internals::types::byte* opgt(viua::internals::types::byte* addr_ptr, int_op target, int_op lhs,
                                           int_op rhs) {
            return emit_instruction_alu(addr_ptr, GT, target, lhs, rhs);
        }
        viua::internals::types::byte* opgte(viua::internals::types::byte* addr_ptr, int_op target, int_op lhs,
                                            int_op rhs) {
            return emit_instruction_alu(addr_ptr, GTE, target, lhs, rhs);
        }
        viua::internals::types::byte* opeq(viua::internals::types::byte* addr_ptr, int_op target, int_op lhs,
                                           int_op rhs) {
            return emit_instruction_alu(addr_ptr, EQ, target, lhs, rhs);
        }

        viua::internals::types::byte* opstrstore(viua::internals::types::byte* addr_ptr, int_op reg,
                                                 string s) {
            *(addr_ptr++) = STRSTORE;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insert_type_prefixed_string(addr_ptr, s.substr(1, s.size() - 2), OT_STRING);
        }

        viua::internals::types::byte* optext(viua::internals::types::byte* addr_ptr, int_op reg, string s) {
            *(addr_ptr++) = TEXT;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insert_type_prefixed_string(addr_ptr, s.substr(1, s.size() - 2), OT_TEXT);
        }

        viua::internals::types::byte* optext(viua::internals::types::byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, TEXT, a, b);
        }

        viua::internals::types::byte* optexteq(viua::internals::types::byte* addr_ptr, int_op target,
                                               int_op lhs, int_op rhs) {
            return insert_three_ri_instruction(addr_ptr, TEXTEQ, target, lhs, rhs);
        }

        viua::internals::types::byte* optextat(viua::internals::types::byte* addr_ptr, int_op target,
                                               int_op source, int_op index) {
            return insert_three_ri_instruction(addr_ptr, TEXTAT, target, source, index);
        }
        viua::internals::types::byte* optextsub(viua::internals::types::byte* addr_ptr, int_op target,
                                                int_op source, int_op begin_index, int_op end_index) {
            return insert_four_ri_instruction(addr_ptr, TEXTSUB, target, source, begin_index, end_index);
        }
        viua::internals::types::byte* optextlength(viua::internals::types::byte* addr_ptr, int_op target,
                                                   int_op source) {
            return insert_two_ri_instruction(addr_ptr, TEXTLENGTH, target, source);
        }
        viua::internals::types::byte* optextcommonprefix(viua::internals::types::byte* addr_ptr,
                                                         int_op target, int_op lhs, int_op rhs) {
            return insert_three_ri_instruction(addr_ptr, TEXTCOMMONPREFIX, target, lhs, rhs);
        }
        viua::internals::types::byte* optextcommonsuffix(viua::internals::types::byte* addr_ptr,
                                                         int_op target, int_op lhs, int_op rhs) {
            return insert_three_ri_instruction(addr_ptr, TEXTCOMMONSUFFIX, target, lhs, rhs);
        }
        viua::internals::types::byte* optextconcat(viua::internals::types::byte* addr_ptr, int_op target,
                                                   int_op lhs, int_op rhs) {
            return insert_three_ri_instruction(addr_ptr, TEXTCONCAT, target, lhs, rhs);
        }

        viua::internals::types::byte* opvec(viua::internals::types::byte* addr_ptr, int_op index,
                                            int_op pack_start_index, int_op pack_length) {
            *(addr_ptr++) = VEC;
            addr_ptr = insert_ri_operand(addr_ptr, index);
            addr_ptr = insert_ri_operand(addr_ptr, pack_start_index);
            return insert_ri_operand(addr_ptr, pack_length);
        }

        viua::internals::types::byte* opvinsert(viua::internals::types::byte* addr_ptr, int_op vec,
                                                int_op src, int_op dst) {
            return insert_three_ri_instruction(addr_ptr, VINSERT, vec, src, dst);
        }

        viua::internals::types::byte* opvpush(viua::internals::types::byte* addr_ptr, int_op vec,
                                              int_op src) {
            return insert_two_ri_instruction(addr_ptr, VPUSH, vec, src);
        }

        viua::internals::types::byte* opvpop(viua::internals::types::byte* addr_ptr, int_op vec, int_op dst,
                                             int_op pos) {
            return insert_three_ri_instruction(addr_ptr, VPOP, vec, dst, pos);
        }

        viua::internals::types::byte* opvat(viua::internals::types::byte* addr_ptr, int_op vec, int_op dst,
                                            int_op at) {
            return insert_three_ri_instruction(addr_ptr, VAT, vec, dst, at);
        }

        viua::internals::types::byte* opvlen(viua::internals::types::byte* addr_ptr, int_op vec, int_op reg) {
            return insert_two_ri_instruction(addr_ptr, VLEN, vec, reg);
        }

        viua::internals::types::byte* opnot(viua::internals::types::byte* addr_ptr, int_op target,
                                            int_op source) {
            *(addr_ptr++) = NOT;
            addr_ptr = insert_ri_operand(addr_ptr, target);
            return insert_ri_operand(addr_ptr, source);
        }

        viua::internals::types::byte* opand(viua::internals::types::byte* addr_ptr, int_op regr, int_op rega,
                                            int_op regb) {
            return insert_three_ri_instruction(addr_ptr, AND, regr, rega, regb);
        }

        viua::internals::types::byte* opor(viua::internals::types::byte* addr_ptr, int_op regr, int_op rega,
                                           int_op regb) {
            return insert_three_ri_instruction(addr_ptr, OR, regr, rega, regb);
        }

        viua::internals::types::byte* opbits(viua::internals::types::byte* addr_ptr, int_op target,
                                             int_op lhs) {
            return insert_two_ri_instruction(addr_ptr, BITS, target, lhs);
        }

        viua::internals::types::byte* opbits(viua::internals::types::byte* addr_ptr, int_op target,
                                             vector<uint8_t> bit_string) {
            *(addr_ptr++) = BITS;
            addr_ptr = insert_ri_operand(addr_ptr, target);
            return insert_size_and_type_prefixed_bitstring(addr_ptr, bit_string);
        }

        viua::internals::types::byte* opbitand(viua::internals::types::byte* addr_ptr, int_op target,
                                               int_op lhs, int_op rhs) {
            return insert_three_ri_instruction(addr_ptr, BITAND, target, lhs, rhs);
        }

        viua::internals::types::byte* opbitor(viua::internals::types::byte* addr_ptr, int_op target,
                                              int_op lhs, int_op rhs) {
            return insert_three_ri_instruction(addr_ptr, BITOR, target, lhs, rhs);
        }

        viua::internals::types::byte* opbitnot(viua::internals::types::byte* addr_ptr, int_op target,
                                               int_op lhs) {
            return insert_two_ri_instruction(addr_ptr, BITNOT, target, lhs);
        }

        viua::internals::types::byte* opbitxor(viua::internals::types::byte* addr_ptr, int_op target,
                                               int_op lhs, int_op rhs) {
            return insert_three_ri_instruction(addr_ptr, BITXOR, target, lhs, rhs);
        }

        viua::internals::types::byte* opbitat(viua::internals::types::byte* addr_ptr, int_op target,
                                              int_op lhs, int_op rhs) {
            return insert_three_ri_instruction(addr_ptr, BITAT, target, lhs, rhs);
        }

        viua::internals::types::byte* opbitset(viua::internals::types::byte* addr_ptr, int_op target,
                                               int_op lhs, int_op rhs) {
            return insert_three_ri_instruction(addr_ptr, BITSET, target, lhs, rhs);
        }

        viua::internals::types::byte* opbitset(viua::internals::types::byte* addr_ptr, int_op target,
                                               int_op lhs, bool rhs) {
            addr_ptr = insert_two_ri_instruction(addr_ptr, BITSET, target, lhs);
            addr_ptr = insert_bool_operand(addr_ptr, rhs);
            return addr_ptr;
        }

        viua::internals::types::byte* opshl(viua::internals::types::byte* addr_ptr, int_op target, int_op lhs,
                                            int_op rhs) {
            return insert_three_ri_instruction(addr_ptr, SHL, target, lhs, rhs);
        }

        viua::internals::types::byte* opshr(viua::internals::types::byte* addr_ptr, int_op target, int_op lhs,
                                            int_op rhs) {
            return insert_three_ri_instruction(addr_ptr, SHR, target, lhs, rhs);
        }

        viua::internals::types::byte* opashl(viua::internals::types::byte* addr_ptr, int_op target,
                                             int_op lhs, int_op rhs) {
            return insert_three_ri_instruction(addr_ptr, ASHL, target, lhs, rhs);
        }

        viua::internals::types::byte* opashr(viua::internals::types::byte* addr_ptr, int_op target,
                                             int_op lhs, int_op rhs) {
            return insert_three_ri_instruction(addr_ptr, ASHR, target, lhs, rhs);
        }

        viua::internals::types::byte* oprol(viua::internals::types::byte* addr_ptr, int_op target,
                                            int_op lhs) {
            return insert_two_ri_instruction(addr_ptr, ROL, target, lhs);
        }

        viua::internals::types::byte* opror(viua::internals::types::byte* addr_ptr, int_op target,
                                            int_op lhs) {
            return insert_two_ri_instruction(addr_ptr, ROR, target, lhs);
        }

        viua::internals::types::byte* opfixedincrement(viua::internals::types::byte* addr_ptr,
                                                       int_op target) {
            *(addr_ptr++) = FIXEDINCREMENT;
            return insert_ri_operand(addr_ptr, target);
        }

        viua::internals::types::byte* opfixeddecrement(viua::internals::types::byte* addr_ptr,
                                                       int_op target) {
            *(addr_ptr++) = FIXEDDECREMENT;
            return insert_ri_operand(addr_ptr, target);
        }

        viua::internals::types::byte* opfixedadd(viua::internals::types::byte* addr_ptr, int_op target,
                                                 int_op lhs, int_op rhs) {
            return insert_three_ri_instruction(addr_ptr, FIXEDADD, target, lhs, rhs);
        }

        viua::internals::types::byte* opfixedmul(viua::internals::types::byte* addr_ptr, int_op target,
                                                 int_op lhs, int_op rhs) {
            return insert_three_ri_instruction(addr_ptr, FIXEDMUL, target, lhs, rhs);
        }


        viua::internals::types::byte* opmove(viua::internals::types::byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, MOVE, a, b);
        }

        viua::internals::types::byte* opcopy(viua::internals::types::byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, COPY, a, b);
        }

        viua::internals::types::byte* opptr(viua::internals::types::byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, PTR, a, b);
        }

        viua::internals::types::byte* opswap(viua::internals::types::byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, SWAP, a, b);
        }

        viua::internals::types::byte* opdelete(viua::internals::types::byte* addr_ptr, int_op reg) {
            *(addr_ptr++) = DELETE;
            return insert_ri_operand(addr_ptr, reg);
        }

        viua::internals::types::byte* opisnull(viua::internals::types::byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, ISNULL, a, b);
        }

        viua::internals::types::byte* opress(viua::internals::types::byte* addr_ptr, const string& a) {
            *(addr_ptr++) = RESS;
            viua::internals::RegisterSets register_set_marker = viua::internals::RegisterSets::LOCAL;
            if (a == "global") {
                register_set_marker = viua::internals::RegisterSets::GLOBAL;
            } else if (a == "local") {
                register_set_marker = viua::internals::RegisterSets::LOCAL;
            } else if (a == "static") {
                register_set_marker = viua::internals::RegisterSets::STATIC;
            } else {
                // FIXME: detect invalid register set names
                // after switching to token-based code generation
                // it will not be necessary
            }
            *(addr_ptr) = static_cast<viua::internals::types::registerset_type_marker>(register_set_marker);
            pointer::inc<viua::internals::types::registerset_type_marker, viua::internals::types::byte>(
                addr_ptr);
            return addr_ptr;
        }

        viua::internals::types::byte* opprint(viua::internals::types::byte* addr_ptr, int_op reg) {
            *(addr_ptr++) = PRINT;
            return insert_ri_operand(addr_ptr, reg);
        }

        viua::internals::types::byte* opecho(viua::internals::types::byte* addr_ptr, int_op reg) {
            *(addr_ptr++) = ECHO;
            return insert_ri_operand(addr_ptr, reg);
        }

        viua::internals::types::byte* opcapture(viua::internals::types::byte* addr_ptr, int_op target_closure,
                                                int_op target_register, int_op source_register) {
            *(addr_ptr++) = CAPTURE;
            addr_ptr = insert_ri_operand(addr_ptr, target_closure);
            addr_ptr = insert_ri_operand(addr_ptr, target_register);
            return insert_ri_operand(addr_ptr, source_register);
        }

        viua::internals::types::byte* opcapturecopy(viua::internals::types::byte* addr_ptr,
                                                    int_op target_closure, int_op target_register,
                                                    int_op source_register) {
            *(addr_ptr++) = CAPTURECOPY;
            addr_ptr = insert_ri_operand(addr_ptr, target_closure);
            addr_ptr = insert_ri_operand(addr_ptr, target_register);
            return insert_ri_operand(addr_ptr, source_register);
        }

        viua::internals::types::byte* opcapturemove(viua::internals::types::byte* addr_ptr,
                                                    int_op target_closure, int_op target_register,
                                                    int_op source_register) {
            *(addr_ptr++) = CAPTUREMOVE;
            addr_ptr = insert_ri_operand(addr_ptr, target_closure);
            addr_ptr = insert_ri_operand(addr_ptr, target_register);
            return insert_ri_operand(addr_ptr, source_register);
        }

        viua::internals::types::byte* opclosure(viua::internals::types::byte* addr_ptr, int_op reg,
                                                const string& fn) {
            *(addr_ptr++) = CLOSURE;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insertString(addr_ptr, fn);
        }

        viua::internals::types::byte* opfunction(viua::internals::types::byte* addr_ptr, int_op reg,
                                                 const string& fn) {
            *(addr_ptr++) = FUNCTION;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insertString(addr_ptr, fn);
        }

        viua::internals::types::byte* opframe(viua::internals::types::byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, FRAME, a, b);
        }

        viua::internals::types::byte* opparam(viua::internals::types::byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, PARAM, a, b);
        }

        viua::internals::types::byte* oppamv(viua::internals::types::byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, PAMV, a, b);
        }

        viua::internals::types::byte* oparg(viua::internals::types::byte* addr_ptr, int_op a, int_op b) {
            return insert_two_ri_instruction(addr_ptr, ARG, a, b);
        }

        viua::internals::types::byte* opargc(viua::internals::types::byte* addr_ptr, int_op a) {
            *(addr_ptr++) = ARGC;
            return insert_ri_operand(addr_ptr, a);
        }

        viua::internals::types::byte* opcall(viua::internals::types::byte* addr_ptr, int_op reg,
                                             const string& fn_name) {
            *(addr_ptr++) = CALL;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insertString(addr_ptr, fn_name);
        }

        viua::internals::types::byte* opcall(viua::internals::types::byte* addr_ptr, int_op reg, int_op fn) {
            return insert_two_ri_instruction(addr_ptr, CALL, reg, fn);
        }

        viua::internals::types::byte* optailcall(viua::internals::types::byte* addr_ptr,
                                                 const string& fn_name) {
            *(addr_ptr++) = TAILCALL;
            return insertString(addr_ptr, fn_name);
        }

        viua::internals::types::byte* optailcall(viua::internals::types::byte* addr_ptr, int_op fn) {
            *(addr_ptr++) = TAILCALL;
            return insert_ri_operand(addr_ptr, fn);
        }

        viua::internals::types::byte* opdefer(viua::internals::types::byte* addr_ptr, const string& fn_name) {
            *(addr_ptr++) = DEFER;
            return insertString(addr_ptr, fn_name);
        }

        viua::internals::types::byte* opdefer(viua::internals::types::byte* addr_ptr, int_op fn) {
            *(addr_ptr++) = DEFER;
            return insert_ri_operand(addr_ptr, fn);
        }

        viua::internals::types::byte* opprocess(viua::internals::types::byte* addr_ptr, int_op reg,
                                                const string& fn_name) {
            *(addr_ptr++) = PROCESS;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insertString(addr_ptr, fn_name);
        }

        viua::internals::types::byte* opprocess(viua::internals::types::byte* addr_ptr, int_op reg,
                                                int_op fn) {
            return insert_two_ri_instruction(addr_ptr, PROCESS, reg, fn);
        }

        viua::internals::types::byte* opself(viua::internals::types::byte* addr_ptr, int_op target) {
            *(addr_ptr++) = SELF;
            return insert_ri_operand(addr_ptr, target);
        }

        viua::internals::types::byte* opjoin(viua::internals::types::byte* addr_ptr, int_op target,
                                             int_op source, timeout_op timeout) {
            *(addr_ptr++) = JOIN;
            addr_ptr = insert_ri_operand(addr_ptr, target);
            addr_ptr = insert_ri_operand(addr_ptr, source);

            // FIXME change to OT_TIMEOUT?
            *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_INT;
            pointer::inc<OperandType, viua::internals::types::byte>(addr_ptr);
            aligned_write(addr_ptr) = timeout.value;
            pointer::inc<viua::internals::types::timeout, viua::internals::types::byte>(addr_ptr);

            return addr_ptr;
        }

        viua::internals::types::byte* opsend(viua::internals::types::byte* addr_ptr, int_op target,
                                             int_op source) {
            *(addr_ptr++) = SEND;
            addr_ptr = insert_ri_operand(addr_ptr, target);
            return insert_ri_operand(addr_ptr, source);
        }

        viua::internals::types::byte* opreceive(viua::internals::types::byte* addr_ptr, int_op reg,
                                                timeout_op timeout) {
            *(addr_ptr++) = RECEIVE;
            addr_ptr = insert_ri_operand(addr_ptr, reg);

            // FIXME change to OT_TIMEOUT?
            *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_INT;
            pointer::inc<OperandType, viua::internals::types::byte>(addr_ptr);

            aligned_write(addr_ptr) = timeout.value;
            pointer::inc<viua::internals::types::timeout, viua::internals::types::byte>(addr_ptr);

            return addr_ptr;
        }

        viua::internals::types::byte* opwatchdog(viua::internals::types::byte* addr_ptr,
                                                 const string& fn_name) {
            *(addr_ptr++) = WATCHDOG;
            return insertString(addr_ptr, fn_name);
        }

        viua::internals::types::byte* opjump(viua::internals::types::byte* addr_ptr,
                                             viua::internals::types::bytecode_size addr) {
            *(addr_ptr++) = JUMP;

            // we *know* that this location in the viua::internals::types::byte array points to
            // viua::internals::types::bytecode_size so
            // the reinterpret_cast<> is justified
            aligned_write(addr_ptr) = addr;
            pointer::inc<viua::internals::types::bytecode_size, viua::internals::types::byte>(addr_ptr);

            return addr_ptr;
        }

        viua::internals::types::byte* opif(viua::internals::types::byte* addr_ptr, int_op regc,
                                           viua::internals::types::bytecode_size addr_truth,
                                           viua::internals::types::bytecode_size addr_false) {
            *(addr_ptr++) = IF;
            addr_ptr = insert_ri_operand(addr_ptr, regc);

            // we *know* that following locations in the viua::internals::types::byte array point to
            // viua::internals::types::bytecode_size so
            // the reinterpret_cast<> is justified
            aligned_write(addr_ptr) = addr_truth;
            pointer::inc<viua::internals::types::bytecode_size, viua::internals::types::byte>(addr_ptr);
            aligned_write(addr_ptr) = addr_false;
            pointer::inc<viua::internals::types::bytecode_size, viua::internals::types::byte>(addr_ptr);

            return addr_ptr;
        }

        viua::internals::types::byte* optry(viua::internals::types::byte* addr_ptr) {
            *(addr_ptr++) = TRY;
            return addr_ptr;
        }

        viua::internals::types::byte* opcatch(viua::internals::types::byte* addr_ptr, const string& type_name,
                                              const string& block_name) {
            *(addr_ptr++) = CATCH;

            // the type
            addr_ptr = insertString(addr_ptr, type_name.substr(1, type_name.size() - 2));

            // catcher block name
            addr_ptr = insertString(addr_ptr, block_name);

            return addr_ptr;
        }

        viua::internals::types::byte* opdraw(viua::internals::types::byte* addr_ptr, int_op regno) {
            *(addr_ptr++) = DRAW;
            return insert_ri_operand(addr_ptr, regno);
        }

        viua::internals::types::byte* openter(viua::internals::types::byte* addr_ptr,
                                              const string& block_name) {
            *(addr_ptr++) = ENTER;
            return insertString(addr_ptr, block_name);
        }

        viua::internals::types::byte* opthrow(viua::internals::types::byte* addr_ptr, int_op regno) {
            *(addr_ptr++) = THROW;
            return insert_ri_operand(addr_ptr, regno);
        }

        viua::internals::types::byte* opleave(viua::internals::types::byte* addr_ptr) {
            *(addr_ptr++) = LEAVE;
            return addr_ptr;
        }

        viua::internals::types::byte* opimport(viua::internals::types::byte* addr_ptr,
                                               const string& module_name) {
            *(addr_ptr++) = IMPORT;
            return insertString(addr_ptr, module_name.substr(1, module_name.size() - 2));
        }

        viua::internals::types::byte* opclass(viua::internals::types::byte* addr_ptr, int_op reg,
                                              const string& class_name) {
            *(addr_ptr++) = CLASS;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insertString(addr_ptr, class_name);
        }

        viua::internals::types::byte* opderive(viua::internals::types::byte* addr_ptr, int_op reg,
                                               const string& base_class_name) {
            *(addr_ptr++) = DERIVE;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insertString(addr_ptr, base_class_name);
        }

        viua::internals::types::byte* opattach(viua::internals::types::byte* addr_ptr, int_op reg,
                                               const string& function_name, const string& method_name) {
            *(addr_ptr++) = ATTACH;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            addr_ptr = insertString(addr_ptr, function_name);
            return insertString(addr_ptr, method_name);
        }

        viua::internals::types::byte* opregister(viua::internals::types::byte* addr_ptr, int_op regno) {
            *(addr_ptr++) = REGISTER;
            return insert_ri_operand(addr_ptr, regno);
        }

        viua::internals::types::byte* opatom(viua::internals::types::byte* addr_ptr, int_op reg, string s) {
            *(addr_ptr++) = ATOM;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insertString(addr_ptr, s.substr(1, s.size() - 2));
        }

        viua::internals::types::byte* opatomeq(viua::internals::types::byte* addr_ptr, int_op target,
                                               int_op lhs, int_op rhs) {
            return insert_three_ri_instruction(addr_ptr, ATOMEQ, target, lhs, rhs);
        }

        viua::internals::types::byte* opstruct(viua::internals::types::byte* addr_ptr, int_op regno) {
            *(addr_ptr++) = STRUCT;
            return insert_ri_operand(addr_ptr, regno);
        }

        viua::internals::types::byte* opstructinsert(viua::internals::types::byte* addr_ptr, int_op rega,
                                                     int_op regb, int_op regr) {
            return insert_three_ri_instruction(addr_ptr, STRUCTINSERT, rega, regb, regr);
        }

        viua::internals::types::byte* opstructremove(viua::internals::types::byte* addr_ptr, int_op rega,
                                                     int_op regb, int_op regr) {
            return insert_three_ri_instruction(addr_ptr, STRUCTREMOVE, rega, regb, regr);
        }

        viua::internals::types::byte* opstructkeys(viua::internals::types::byte* addr_ptr, int_op target,
                                                   int_op source) {
            return insert_two_ri_instruction(addr_ptr, STRUCTKEYS, target, source);
        }

        viua::internals::types::byte* opnew(viua::internals::types::byte* addr_ptr, int_op reg,
                                            const string& class_name) {
            *(addr_ptr++) = NEW;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insertString(addr_ptr, class_name);
        }

        viua::internals::types::byte* opmsg(viua::internals::types::byte* addr_ptr, int_op reg,
                                            const string& method_name) {
            *(addr_ptr++) = MSG;
            addr_ptr = insert_ri_operand(addr_ptr, reg);
            return insertString(addr_ptr, method_name);
        }

        viua::internals::types::byte* opmsg(viua::internals::types::byte* addr_ptr, int_op reg,
                                            int_op method_name) {
            return insert_two_ri_instruction(addr_ptr, MSG, reg, method_name);
        }

        viua::internals::types::byte* opinsert(viua::internals::types::byte* addr_ptr, int_op rega,
                                               int_op regb, int_op regr) {
            return insert_three_ri_instruction(addr_ptr, INSERT, rega, regb, regr);
        }

        viua::internals::types::byte* opremove(viua::internals::types::byte* addr_ptr, int_op rega,
                                               int_op regb, int_op regr) {
            return insert_three_ri_instruction(addr_ptr, REMOVE, rega, regb, regr);
        }

        viua::internals::types::byte* opreturn(viua::internals::types::byte* addr_ptr) {
            *(addr_ptr++) = RETURN;
            return addr_ptr;
        }

        viua::internals::types::byte* ophalt(viua::internals::types::byte* addr_ptr) {
            *(addr_ptr++) = HALT;
            return addr_ptr;
        }
    }
}
