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
        : type(IntegerOperandType::PLAIN)
        , rs_type(viua::internals::RegisterSets::CURRENT)
        , value(0) {}
int_op::int_op(IntegerOperandType t, viua::internals::types::plain_int n)
        : type(t), rs_type(viua::internals::RegisterSets::CURRENT), value(n) {}
int_op::int_op(IntegerOperandType t,
               viua::internals::RegisterSets rst,
               viua::internals::types::plain_int n)
        : type(t), rs_type(rst), value(n) {}
int_op::int_op(viua::internals::types::plain_int n)
        : type(IntegerOperandType::PLAIN)
        , rs_type(viua::internals::RegisterSets::CURRENT)
        , value(n) {}

timeout_op::timeout_op() : type(IntegerOperandType::PLAIN), value(0) {}
timeout_op::timeout_op(IntegerOperandType t, viua::internals::types::timeout n)
        : type(t), value(n) {}
timeout_op::timeout_op(viua::internals::types::timeout n)
        : type(IntegerOperandType::PLAIN), value(n) {}


static auto insert_ri_operand(viua::internals::types::byte* addr_ptr, int_op op)
    -> viua::internals::types::byte* {
    /** Insert integer operand into bytecode.
     *
     *  When using integer operand, it usually is a plain number - which
     * translates to a regsiter index. However, when preceded by `@` integer
     * operand will not be interpreted directly, but instead
     * viua::kernel::Kernel
     *  will look into a register the integer points to, fetch an integer from
     * this register and use the fetched register as the operand.
     */
    if (op.type == IntegerOperandType::VOID) {
        *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_VOID;
        pointer::inc<OperandType, viua::internals::types::byte>(addr_ptr);
        return addr_ptr;
    }

    /* NOTICE: reinterpret_cast<>'s are ugly, but we know what we're doing here.
     * Since we store everything in a big array of bytes we have to cast
     * incompatible pointers to actually put *valid* data inside it.
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
    pointer::inc<viua::internals::types::register_index,
                 viua::internals::types::byte>(addr_ptr);

    *(reinterpret_cast<viua::internals::RegisterSets*>(addr_ptr)) = op.rs_type;
    pointer::inc<viua::internals::RegisterSets, viua::internals::types::byte>(
        addr_ptr);

    return addr_ptr;
}

static auto insert_bool_operand(viua::internals::types::byte* addr_ptr, bool op)
    -> viua::internals::types::byte* {
    if (op) {
        *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_TRUE;
    } else {
        *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_FALSE;
    }
    pointer::inc<OperandType, viua::internals::types::byte>(addr_ptr);

    return addr_ptr;
}

static auto insert_two_ri_instruction(viua::internals::types::byte* addr_ptr,
                                      enum OPCODE instruction,
                                      int_op a,
                                      int_op b)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = instruction;
    addr_ptr      = insert_ri_operand(addr_ptr, a);
    return insert_ri_operand(addr_ptr, b);
}

static auto insert_three_ri_instruction(viua::internals::types::byte* addr_ptr,
                                        enum OPCODE instruction,
                                        int_op a,
                                        int_op b,
                                        int_op c)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = instruction;
    addr_ptr      = insert_ri_operand(addr_ptr, a);
    addr_ptr      = insert_ri_operand(addr_ptr, b);
    return insert_ri_operand(addr_ptr, c);
}

static auto insert_four_ri_instruction(viua::internals::types::byte* addr_ptr,
                                       enum OPCODE instruction,
                                       int_op a,
                                       int_op b,
                                       int_op c,
                                       int_op d)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = instruction;
    addr_ptr      = insert_ri_operand(addr_ptr, a);
    addr_ptr      = insert_ri_operand(addr_ptr, b);
    addr_ptr      = insert_ri_operand(addr_ptr, c);
    return insert_ri_operand(addr_ptr, d);
}

static auto insert_type_prefixed_string(viua::internals::types::byte* ptr,
                                        const string& s,
                                        const OperandType op_type)
    -> viua::internals::types::byte* {
    *(reinterpret_cast<OperandType*>(ptr)) = op_type;
    pointer::inc<OperandType, viua::internals::types::byte>(ptr);
    for (auto const each : s) {
        *(ptr++) = static_cast<viua::internals::types::byte>(each);
    }
    *(ptr++) = '\0';
    return ptr;
}

static auto insert_size_and_type_prefixed_bitstring(
    viua::internals::types::byte* ptr,
    const vector<uint8_t> bit_string) -> viua::internals::types::byte* {
    *(reinterpret_cast<OperandType*>(ptr)) = OT_BITS;
    pointer::inc<OperandType, viua::internals::types::byte>(ptr);
    aligned_write(ptr) = bit_string.size();
    pointer::inc<viua::internals::types::bits_size,
                 viua::internals::types::byte>(ptr);

    /*
     * Binaries (and other multibyte values) are stored in the Big Endian
     * encoding. Remember about this!
     */
    for (auto i = bit_string.size(); i; --i) {
        *ptr = bit_string.at(i - 1);
        ++ptr;
    }

    return ptr;
}

static auto insert_string(viua::internals::types::byte* ptr, const string& s)
    -> viua::internals::types::byte* {
    for (auto const each : s) {
        *(ptr++) = static_cast<viua::internals::types::byte>(each);
    }
    *(ptr++) = '\0';
    return ptr;
}

namespace cg { namespace bytecode {
auto opnop(viua::internals::types::byte* addr_ptr)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = NOP;
    return addr_ptr;
}

auto opizero(viua::internals::types::byte* addr_ptr, int_op regno)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = IZERO;
    return insert_ri_operand(addr_ptr, regno);
}

auto opinteger(viua::internals::types::byte* addr_ptr, int_op regno, int_op i)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = INTEGER;
    addr_ptr      = insert_ri_operand(addr_ptr, regno);

    *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_INT;
    pointer::inc<OperandType, viua::internals::types::byte>(addr_ptr);
    aligned_write(addr_ptr) = i.value;
    pointer::inc<viua::internals::types::plain_int,
                 viua::internals::types::byte>(addr_ptr);

    return addr_ptr;
}

auto opiinc(viua::internals::types::byte* addr_ptr, int_op regno)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = IINC;
    return insert_ri_operand(addr_ptr, regno);
}

auto opidec(viua::internals::types::byte* addr_ptr, int_op regno)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = IDEC;
    return insert_ri_operand(addr_ptr, regno);
}

auto opfloat(viua::internals::types::byte* addr_ptr,
             int_op regno,
             viua::internals::types::plain_float f)
    -> viua::internals::types::byte* {
    *(addr_ptr++)           = FLOAT;
    addr_ptr                = insert_ri_operand(addr_ptr, regno);
    aligned_write(addr_ptr) = f;
    pointer::inc<viua::internals::types::plain_float,
                 viua::internals::types::byte>(addr_ptr);

    return addr_ptr;
}

auto opitof(viua::internals::types::byte* addr_ptr, int_op a, int_op b)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, ITOF, a, b);
}

auto opftoi(viua::internals::types::byte* addr_ptr, int_op a, int_op b)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, FTOI, a, b);
}

auto opstoi(viua::internals::types::byte* addr_ptr, int_op a, int_op b)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, STOI, a, b);
}

auto opstof(viua::internals::types::byte* addr_ptr, int_op a, int_op b)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, STOF, a, b);
}

static auto emit_instruction_alu(viua::internals::types::byte* addr_ptr,
                                 OPCODE instruction,
                                 int_op target,
                                 int_op lhs,
                                 int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, instruction, target, lhs, rhs);
}
auto opadd(viua::internals::types::byte* addr_ptr,
           int_op target,
           int_op lhs,
           int_op rhs) -> viua::internals::types::byte* {
    return emit_instruction_alu(addr_ptr, ADD, target, lhs, rhs);
}
auto opsub(viua::internals::types::byte* addr_ptr,
           int_op target,
           int_op lhs,
           int_op rhs) -> viua::internals::types::byte* {
    return emit_instruction_alu(addr_ptr, SUB, target, lhs, rhs);
}
auto opmul(viua::internals::types::byte* addr_ptr,
           int_op target,
           int_op lhs,
           int_op rhs) -> viua::internals::types::byte* {
    return emit_instruction_alu(addr_ptr, MUL, target, lhs, rhs);
}
auto opdiv(viua::internals::types::byte* addr_ptr,
           int_op target,
           int_op lhs,
           int_op rhs) -> viua::internals::types::byte* {
    return emit_instruction_alu(addr_ptr, DIV, target, lhs, rhs);
}
auto oplt(viua::internals::types::byte* addr_ptr,
          int_op target,
          int_op lhs,
          int_op rhs) -> viua::internals::types::byte* {
    return emit_instruction_alu(addr_ptr, LT, target, lhs, rhs);
}
auto oplte(viua::internals::types::byte* addr_ptr,
           int_op target,
           int_op lhs,
           int_op rhs) -> viua::internals::types::byte* {
    return emit_instruction_alu(addr_ptr, LTE, target, lhs, rhs);
}
auto opgt(viua::internals::types::byte* addr_ptr,
          int_op target,
          int_op lhs,
          int_op rhs) -> viua::internals::types::byte* {
    return emit_instruction_alu(addr_ptr, GT, target, lhs, rhs);
}
auto opgte(viua::internals::types::byte* addr_ptr,
           int_op target,
           int_op lhs,
           int_op rhs) -> viua::internals::types::byte* {
    return emit_instruction_alu(addr_ptr, GTE, target, lhs, rhs);
}
auto opeq(viua::internals::types::byte* addr_ptr,
          int_op target,
          int_op lhs,
          int_op rhs) -> viua::internals::types::byte* {
    return emit_instruction_alu(addr_ptr, EQ, target, lhs, rhs);
}

auto opstring(viua::internals::types::byte* addr_ptr, int_op reg, string s)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = STRING;
    addr_ptr      = insert_ri_operand(addr_ptr, reg);
    return insert_type_prefixed_string(
        addr_ptr, s.substr(1, s.size() - 2), OT_STRING);
}

auto optext(viua::internals::types::byte* addr_ptr, int_op reg, string s)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = TEXT;
    addr_ptr      = insert_ri_operand(addr_ptr, reg);
    return insert_type_prefixed_string(
        addr_ptr, s.substr(1, s.size() - 2), OT_TEXT);
}

auto optext(viua::internals::types::byte* addr_ptr, int_op a, int_op b)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, TEXT, a, b);
}

auto optexteq(viua::internals::types::byte* addr_ptr,
              int_op target,
              int_op lhs,
              int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, TEXTEQ, target, lhs, rhs);
}

auto optextat(viua::internals::types::byte* addr_ptr,
              int_op target,
              int_op source,
              int_op index) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, TEXTAT, target, source, index);
}
auto optextsub(viua::internals::types::byte* addr_ptr,
               int_op target,
               int_op source,
               int_op begin_index,
               int_op end_index) -> viua::internals::types::byte* {
    return insert_four_ri_instruction(
        addr_ptr, TEXTSUB, target, source, begin_index, end_index);
}
auto optextlength(viua::internals::types::byte* addr_ptr,
                  int_op target,
                  int_op source) -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, TEXTLENGTH, target, source);
}
auto optextcommonprefix(viua::internals::types::byte* addr_ptr,
                        int_op target,
                        int_op lhs,
                        int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(
        addr_ptr, TEXTCOMMONPREFIX, target, lhs, rhs);
}
auto optextcommonsuffix(viua::internals::types::byte* addr_ptr,
                        int_op target,
                        int_op lhs,
                        int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(
        addr_ptr, TEXTCOMMONSUFFIX, target, lhs, rhs);
}
auto optextconcat(viua::internals::types::byte* addr_ptr,
                  int_op target,
                  int_op lhs,
                  int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, TEXTCONCAT, target, lhs, rhs);
}

auto opvector(viua::internals::types::byte* addr_ptr,
              int_op index,
              int_op pack_start_index,
              int_op pack_length) -> viua::internals::types::byte* {
    *(addr_ptr++) = VECTOR;
    addr_ptr      = insert_ri_operand(addr_ptr, index);
    addr_ptr      = insert_ri_operand(addr_ptr, pack_start_index);
    return insert_ri_operand(addr_ptr, pack_length);
}

auto opvinsert(viua::internals::types::byte* addr_ptr,
               int_op vec,
               int_op src,
               int_op dst) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, VINSERT, vec, src, dst);
}

auto opvpush(viua::internals::types::byte* addr_ptr, int_op vec, int_op src)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, VPUSH, vec, src);
}

auto opvpop(viua::internals::types::byte* addr_ptr,
            int_op vec,
            int_op dst,
            int_op pos) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, VPOP, vec, dst, pos);
}

auto opvat(viua::internals::types::byte* addr_ptr,
           int_op vec,
           int_op dst,
           int_op at) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, VAT, vec, dst, at);
}

auto opvlen(viua::internals::types::byte* addr_ptr, int_op vec, int_op reg)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, VLEN, vec, reg);
}

auto opnot(viua::internals::types::byte* addr_ptr, int_op target, int_op source)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = NOT;
    addr_ptr      = insert_ri_operand(addr_ptr, target);
    return insert_ri_operand(addr_ptr, source);
}

auto opand(viua::internals::types::byte* addr_ptr,
           int_op regr,
           int_op rega,
           int_op regb) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, AND, regr, rega, regb);
}

auto opor(viua::internals::types::byte* addr_ptr,
          int_op regr,
          int_op rega,
          int_op regb) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, OR, regr, rega, regb);
}

auto opbits(viua::internals::types::byte* addr_ptr, int_op target, int_op lhs)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, BITS, target, lhs);
}

auto opbits(viua::internals::types::byte* addr_ptr,
            int_op target,
            vector<uint8_t> bit_string) -> viua::internals::types::byte* {
    *(addr_ptr++) = BITS;
    addr_ptr      = insert_ri_operand(addr_ptr, target);
    return insert_size_and_type_prefixed_bitstring(addr_ptr, bit_string);
}

auto opbitand(viua::internals::types::byte* addr_ptr,
              int_op target,
              int_op lhs,
              int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, BITAND, target, lhs, rhs);
}

auto opbitor(viua::internals::types::byte* addr_ptr,
             int_op target,
             int_op lhs,
             int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, BITOR, target, lhs, rhs);
}

auto opbitnot(viua::internals::types::byte* addr_ptr, int_op target, int_op lhs)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, BITNOT, target, lhs);
}

auto opbitxor(viua::internals::types::byte* addr_ptr,
              int_op target,
              int_op lhs,
              int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, BITXOR, target, lhs, rhs);
}

auto opbitat(viua::internals::types::byte* addr_ptr,
             int_op target,
             int_op lhs,
             int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, BITAT, target, lhs, rhs);
}

auto opbitset(viua::internals::types::byte* addr_ptr,
              int_op target,
              int_op lhs,
              int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, BITSET, target, lhs, rhs);
}

auto opbitset(viua::internals::types::byte* addr_ptr,
              int_op target,
              int_op lhs,
              bool rhs) -> viua::internals::types::byte* {
    addr_ptr = insert_two_ri_instruction(addr_ptr, BITSET, target, lhs);
    addr_ptr = insert_bool_operand(addr_ptr, rhs);
    return addr_ptr;
}

auto opshl(viua::internals::types::byte* addr_ptr,
           int_op target,
           int_op lhs,
           int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, SHL, target, lhs, rhs);
}

auto opshr(viua::internals::types::byte* addr_ptr,
           int_op target,
           int_op lhs,
           int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, SHR, target, lhs, rhs);
}

auto opashl(viua::internals::types::byte* addr_ptr,
            int_op target,
            int_op lhs,
            int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, ASHL, target, lhs, rhs);
}

auto opashr(viua::internals::types::byte* addr_ptr,
            int_op target,
            int_op lhs,
            int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, ASHR, target, lhs, rhs);
}

auto oprol(viua::internals::types::byte* addr_ptr, int_op target, int_op lhs)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, ROL, target, lhs);
}

auto opror(viua::internals::types::byte* addr_ptr, int_op target, int_op lhs)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, ROR, target, lhs);
}

auto opwrapincrement(viua::internals::types::byte* addr_ptr, int_op target)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = WRAPINCREMENT;
    return insert_ri_operand(addr_ptr, target);
}

auto opwrapdecrement(viua::internals::types::byte* addr_ptr, int_op target)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = WRAPDECREMENT;
    return insert_ri_operand(addr_ptr, target);
}

auto opwrapadd(viua::internals::types::byte* addr_ptr,
               int_op target,
               int_op lhs,
               int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, WRAPADD, target, lhs, rhs);
}

auto opwrapsub(viua::internals::types::byte* addr_ptr,
               int_op target,
               int_op lhs,
               int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, WRAPSUB, target, lhs, rhs);
}

auto opwrapmul(viua::internals::types::byte* addr_ptr,
               int_op target,
               int_op lhs,
               int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, WRAPMUL, target, lhs, rhs);
}

auto opwrapdiv(viua::internals::types::byte* addr_ptr,
               int_op target,
               int_op lhs,
               int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, WRAPDIV, target, lhs, rhs);
}

auto opcheckedsincrement(viua::internals::types::byte* addr_ptr, int_op target)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = CHECKEDSINCREMENT;
    return insert_ri_operand(addr_ptr, target);
}

auto opcheckedsdecrement(viua::internals::types::byte* addr_ptr, int_op target)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = CHECKEDSDECREMENT;
    return insert_ri_operand(addr_ptr, target);
}

auto opcheckedsadd(viua::internals::types::byte* addr_ptr,
                   int_op target,
                   int_op lhs,
                   int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, CHECKEDSADD, target, lhs, rhs);
}

auto opcheckedssub(viua::internals::types::byte* addr_ptr,
                   int_op target,
                   int_op lhs,
                   int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, CHECKEDSSUB, target, lhs, rhs);
}

auto opcheckedsmul(viua::internals::types::byte* addr_ptr,
                   int_op target,
                   int_op lhs,
                   int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, CHECKEDSMUL, target, lhs, rhs);
}

auto opcheckedsdiv(viua::internals::types::byte* addr_ptr,
                   int_op target,
                   int_op lhs,
                   int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, CHECKEDSDIV, target, lhs, rhs);
}

auto opcheckeduincrement(viua::internals::types::byte* addr_ptr, int_op target)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = CHECKEDUINCREMENT;
    return insert_ri_operand(addr_ptr, target);
}

auto opcheckedudecrement(viua::internals::types::byte* addr_ptr, int_op target)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = CHECKEDUDECREMENT;
    return insert_ri_operand(addr_ptr, target);
}

auto opcheckeduadd(viua::internals::types::byte* addr_ptr,
                   int_op target,
                   int_op lhs,
                   int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, CHECKEDUADD, target, lhs, rhs);
}

auto opcheckedusub(viua::internals::types::byte* addr_ptr,
                   int_op target,
                   int_op lhs,
                   int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, CHECKEDUSUB, target, lhs, rhs);
}

auto opcheckedumul(viua::internals::types::byte* addr_ptr,
                   int_op target,
                   int_op lhs,
                   int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, CHECKEDUMUL, target, lhs, rhs);
}

auto opcheckedudiv(viua::internals::types::byte* addr_ptr,
                   int_op target,
                   int_op lhs,
                   int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, CHECKEDUDIV, target, lhs, rhs);
}

auto opsaturatingsincrement(viua::internals::types::byte* addr_ptr,
                            int_op target) -> viua::internals::types::byte* {
    *(addr_ptr++) = SATURATINGSINCREMENT;
    return insert_ri_operand(addr_ptr, target);
}

auto opsaturatingsdecrement(viua::internals::types::byte* addr_ptr,
                            int_op target) -> viua::internals::types::byte* {
    *(addr_ptr++) = SATURATINGSDECREMENT;
    return insert_ri_operand(addr_ptr, target);
}

auto opsaturatingsadd(viua::internals::types::byte* addr_ptr,
                      int_op target,
                      int_op lhs,
                      int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(
        addr_ptr, SATURATINGSADD, target, lhs, rhs);
}

auto opsaturatingssub(viua::internals::types::byte* addr_ptr,
                      int_op target,
                      int_op lhs,
                      int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(
        addr_ptr, SATURATINGSSUB, target, lhs, rhs);
}

auto opsaturatingsmul(viua::internals::types::byte* addr_ptr,
                      int_op target,
                      int_op lhs,
                      int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(
        addr_ptr, SATURATINGSMUL, target, lhs, rhs);
}

auto opsaturatingsdiv(viua::internals::types::byte* addr_ptr,
                      int_op target,
                      int_op lhs,
                      int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(
        addr_ptr, SATURATINGSDIV, target, lhs, rhs);
}

auto opsaturatinguincrement(viua::internals::types::byte* addr_ptr,
                            int_op target) -> viua::internals::types::byte* {
    *(addr_ptr++) = SATURATINGUINCREMENT;
    return insert_ri_operand(addr_ptr, target);
}

auto opsaturatingudecrement(viua::internals::types::byte* addr_ptr,
                            int_op target) -> viua::internals::types::byte* {
    *(addr_ptr++) = SATURATINGUDECREMENT;
    return insert_ri_operand(addr_ptr, target);
}

auto opsaturatinguadd(viua::internals::types::byte* addr_ptr,
                      int_op target,
                      int_op lhs,
                      int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(
        addr_ptr, SATURATINGUADD, target, lhs, rhs);
}

auto opsaturatingusub(viua::internals::types::byte* addr_ptr,
                      int_op target,
                      int_op lhs,
                      int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(
        addr_ptr, SATURATINGUSUB, target, lhs, rhs);
}

auto opsaturatingumul(viua::internals::types::byte* addr_ptr,
                      int_op target,
                      int_op lhs,
                      int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(
        addr_ptr, SATURATINGUMUL, target, lhs, rhs);
}

auto opsaturatingudiv(viua::internals::types::byte* addr_ptr,
                      int_op target,
                      int_op lhs,
                      int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(
        addr_ptr, SATURATINGUDIV, target, lhs, rhs);
}

auto opmove(viua::internals::types::byte* addr_ptr, int_op a, int_op b)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, MOVE, a, b);
}

auto opcopy(viua::internals::types::byte* addr_ptr, int_op a, int_op b)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, COPY, a, b);
}

auto opptr(viua::internals::types::byte* addr_ptr, int_op a, int_op b)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, PTR, a, b);
}

auto opswap(viua::internals::types::byte* addr_ptr, int_op a, int_op b)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, SWAP, a, b);
}

auto opdelete(viua::internals::types::byte* addr_ptr, int_op reg)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = DELETE;
    return insert_ri_operand(addr_ptr, reg);
}

auto opisnull(viua::internals::types::byte* addr_ptr, int_op a, int_op b)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, ISNULL, a, b);
}

auto opprint(viua::internals::types::byte* addr_ptr, int_op reg)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = PRINT;
    return insert_ri_operand(addr_ptr, reg);
}

auto opecho(viua::internals::types::byte* addr_ptr, int_op reg)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = ECHO;
    return insert_ri_operand(addr_ptr, reg);
}

auto opcapture(viua::internals::types::byte* addr_ptr,
               int_op target_closure,
               int_op target_register,
               int_op source_register) -> viua::internals::types::byte* {
    *(addr_ptr++) = CAPTURE;
    addr_ptr      = insert_ri_operand(addr_ptr, target_closure);
    addr_ptr      = insert_ri_operand(addr_ptr, target_register);
    return insert_ri_operand(addr_ptr, source_register);
}

auto opcapturecopy(viua::internals::types::byte* addr_ptr,
                   int_op target_closure,
                   int_op target_register,
                   int_op source_register) -> viua::internals::types::byte* {
    *(addr_ptr++) = CAPTURECOPY;
    addr_ptr      = insert_ri_operand(addr_ptr, target_closure);
    addr_ptr      = insert_ri_operand(addr_ptr, target_register);
    return insert_ri_operand(addr_ptr, source_register);
}

auto opcapturemove(viua::internals::types::byte* addr_ptr,
                   int_op target_closure,
                   int_op target_register,
                   int_op source_register) -> viua::internals::types::byte* {
    *(addr_ptr++) = CAPTUREMOVE;
    addr_ptr      = insert_ri_operand(addr_ptr, target_closure);
    addr_ptr      = insert_ri_operand(addr_ptr, target_register);
    return insert_ri_operand(addr_ptr, source_register);
}

auto opclosure(viua::internals::types::byte* addr_ptr,
               int_op reg,
               const string& fn) -> viua::internals::types::byte* {
    *(addr_ptr++) = CLOSURE;
    addr_ptr      = insert_ri_operand(addr_ptr, reg);
    return insert_string(addr_ptr, fn);
}

auto opfunction(viua::internals::types::byte* addr_ptr,
                int_op reg,
                const string& fn) -> viua::internals::types::byte* {
    *(addr_ptr++) = FUNCTION;
    addr_ptr      = insert_ri_operand(addr_ptr, reg);
    return insert_string(addr_ptr, fn);
}

auto opframe(viua::internals::types::byte* addr_ptr, int_op a, int_op b)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, FRAME, a, b);
}

auto opparam(viua::internals::types::byte* addr_ptr, int_op a, int_op b)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, PARAM, a, b);
}

auto oppamv(viua::internals::types::byte* addr_ptr, int_op a, int_op b)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, PAMV, a, b);
}

auto oparg(viua::internals::types::byte* addr_ptr, int_op a, int_op b)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, ARG, a, b);
}

auto opargc(viua::internals::types::byte* addr_ptr, int_op a)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = ARGC;
    return insert_ri_operand(addr_ptr, a);
}

auto opcall(viua::internals::types::byte* addr_ptr,
            int_op reg,
            const string& fn_name) -> viua::internals::types::byte* {
    *(addr_ptr++) = CALL;
    addr_ptr      = insert_ri_operand(addr_ptr, reg);
    return insert_string(addr_ptr, fn_name);
}

auto opcall(viua::internals::types::byte* addr_ptr, int_op reg, int_op fn)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, CALL, reg, fn);
}

auto optailcall(viua::internals::types::byte* addr_ptr, const string& fn_name)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = TAILCALL;
    return insert_string(addr_ptr, fn_name);
}

auto optailcall(viua::internals::types::byte* addr_ptr, int_op fn)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = TAILCALL;
    return insert_ri_operand(addr_ptr, fn);
}

auto opdefer(viua::internals::types::byte* addr_ptr, const string& fn_name)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = DEFER;
    return insert_string(addr_ptr, fn_name);
}

auto opdefer(viua::internals::types::byte* addr_ptr, int_op fn)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = DEFER;
    return insert_ri_operand(addr_ptr, fn);
}

auto opprocess(viua::internals::types::byte* addr_ptr,
               int_op reg,
               const string& fn_name) -> viua::internals::types::byte* {
    *(addr_ptr++) = PROCESS;
    addr_ptr      = insert_ri_operand(addr_ptr, reg);
    return insert_string(addr_ptr, fn_name);
}

auto opprocess(viua::internals::types::byte* addr_ptr, int_op reg, int_op fn)
    -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, PROCESS, reg, fn);
}

auto opself(viua::internals::types::byte* addr_ptr, int_op target)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = SELF;
    return insert_ri_operand(addr_ptr, target);
}

auto opjoin(viua::internals::types::byte* addr_ptr,
            int_op target,
            int_op source,
            timeout_op timeout) -> viua::internals::types::byte* {
    *(addr_ptr++) = JOIN;
    addr_ptr      = insert_ri_operand(addr_ptr, target);
    addr_ptr      = insert_ri_operand(addr_ptr, source);

    // FIXME change to OT_TIMEOUT?
    *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_INT;
    pointer::inc<OperandType, viua::internals::types::byte>(addr_ptr);
    aligned_write(addr_ptr) = timeout.value;
    pointer::inc<viua::internals::types::timeout, viua::internals::types::byte>(
        addr_ptr);

    return addr_ptr;
}

auto opsend(viua::internals::types::byte* addr_ptr,
            int_op target,
            int_op source) -> viua::internals::types::byte* {
    *(addr_ptr++) = SEND;
    addr_ptr      = insert_ri_operand(addr_ptr, target);
    return insert_ri_operand(addr_ptr, source);
}

auto opreceive(viua::internals::types::byte* addr_ptr,
               int_op reg,
               timeout_op timeout) -> viua::internals::types::byte* {
    *(addr_ptr++) = RECEIVE;
    addr_ptr      = insert_ri_operand(addr_ptr, reg);

    // FIXME change to OT_TIMEOUT?
    *(reinterpret_cast<OperandType*>(addr_ptr)) = OT_INT;
    pointer::inc<OperandType, viua::internals::types::byte>(addr_ptr);

    aligned_write(addr_ptr) = timeout.value;
    pointer::inc<viua::internals::types::timeout, viua::internals::types::byte>(
        addr_ptr);

    return addr_ptr;
}

auto opwatchdog(viua::internals::types::byte* addr_ptr, const string& fn_name)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = WATCHDOG;
    return insert_string(addr_ptr, fn_name);
}

auto opjump(viua::internals::types::byte* addr_ptr,
            viua::internals::types::bytecode_size addr)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = JUMP;

    // we *know* that this location in the viua::internals::types::byte array
    // points to viua::internals::types::bytecode_size so the reinterpret_cast<>
    // is justified
    aligned_write(addr_ptr) = addr;
    pointer::inc<viua::internals::types::bytecode_size,
                 viua::internals::types::byte>(addr_ptr);

    return addr_ptr;
}

auto opif(viua::internals::types::byte* addr_ptr,
          int_op regc,
          viua::internals::types::bytecode_size addr_truth,
          viua::internals::types::bytecode_size addr_false)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = IF;
    addr_ptr      = insert_ri_operand(addr_ptr, regc);

    // we *know* that following locations in the viua::internals::types::byte
    // array point to viua::internals::types::bytecode_size so the
    // reinterpret_cast<> is justified
    aligned_write(addr_ptr) = addr_truth;
    pointer::inc<viua::internals::types::bytecode_size,
                 viua::internals::types::byte>(addr_ptr);
    aligned_write(addr_ptr) = addr_false;
    pointer::inc<viua::internals::types::bytecode_size,
                 viua::internals::types::byte>(addr_ptr);

    return addr_ptr;
}

auto optry(viua::internals::types::byte* addr_ptr)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = TRY;
    return addr_ptr;
}

auto opcatch(viua::internals::types::byte* addr_ptr,
             const string& type_name,
             const string& block_name) -> viua::internals::types::byte* {
    *(addr_ptr++) = CATCH;

    // the type
    addr_ptr =
        insert_string(addr_ptr, type_name.substr(1, type_name.size() - 2));

    // catcher block name
    addr_ptr = insert_string(addr_ptr, block_name);

    return addr_ptr;
}

auto opdraw(viua::internals::types::byte* addr_ptr, int_op regno)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = DRAW;
    return insert_ri_operand(addr_ptr, regno);
}

auto openter(viua::internals::types::byte* addr_ptr, const string& block_name)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = ENTER;
    return insert_string(addr_ptr, block_name);
}

auto opthrow(viua::internals::types::byte* addr_ptr, int_op regno)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = THROW;
    return insert_ri_operand(addr_ptr, regno);
}

auto opleave(viua::internals::types::byte* addr_ptr)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = LEAVE;
    return addr_ptr;
}

auto opimport(viua::internals::types::byte* addr_ptr, const string& module_name)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = IMPORT;
    return insert_string(addr_ptr,
                         module_name.substr(1, module_name.size() - 2));
}

auto opclass(viua::internals::types::byte* addr_ptr,
             int_op reg,
             const string& class_name) -> viua::internals::types::byte* {
    *(addr_ptr++) = CLASS;
    addr_ptr      = insert_ri_operand(addr_ptr, reg);
    return insert_string(addr_ptr, class_name);
}

auto opderive(viua::internals::types::byte* addr_ptr,
              int_op reg,
              const string& base_class_name) -> viua::internals::types::byte* {
    *(addr_ptr++) = DERIVE;
    addr_ptr      = insert_ri_operand(addr_ptr, reg);
    return insert_string(addr_ptr, base_class_name);
}

auto opattach(viua::internals::types::byte* addr_ptr,
              int_op reg,
              const string& function_name,
              const string& method_name) -> viua::internals::types::byte* {
    *(addr_ptr++) = ATTACH;
    addr_ptr      = insert_ri_operand(addr_ptr, reg);
    addr_ptr      = insert_string(addr_ptr, function_name);
    return insert_string(addr_ptr, method_name);
}

auto opregister(viua::internals::types::byte* addr_ptr, int_op regno)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = REGISTER;
    return insert_ri_operand(addr_ptr, regno);
}

auto opatom(viua::internals::types::byte* addr_ptr, int_op reg, string s)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = ATOM;
    addr_ptr      = insert_ri_operand(addr_ptr, reg);
    return insert_string(addr_ptr, s.substr(1, s.size() - 2));
}

auto opatomeq(viua::internals::types::byte* addr_ptr,
              int_op target,
              int_op lhs,
              int_op rhs) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, ATOMEQ, target, lhs, rhs);
}

auto opstruct(viua::internals::types::byte* addr_ptr, int_op regno)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = STRUCT;
    return insert_ri_operand(addr_ptr, regno);
}

auto opstructinsert(viua::internals::types::byte* addr_ptr,
                    int_op rega,
                    int_op regb,
                    int_op regr) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(
        addr_ptr, STRUCTINSERT, rega, regb, regr);
}

auto opstructremove(viua::internals::types::byte* addr_ptr,
                    int_op rega,
                    int_op regb,
                    int_op regr) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(
        addr_ptr, STRUCTREMOVE, rega, regb, regr);
}

auto opstructkeys(viua::internals::types::byte* addr_ptr,
                  int_op target,
                  int_op source) -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, STRUCTKEYS, target, source);
}

auto opnew(viua::internals::types::byte* addr_ptr,
           int_op reg,
           const string& class_name) -> viua::internals::types::byte* {
    *(addr_ptr++) = NEW;
    addr_ptr      = insert_ri_operand(addr_ptr, reg);
    return insert_string(addr_ptr, class_name);
}

auto opmsg(viua::internals::types::byte* addr_ptr,
           int_op reg,
           const string& method_name) -> viua::internals::types::byte* {
    *(addr_ptr++) = MSG;
    addr_ptr      = insert_ri_operand(addr_ptr, reg);
    return insert_string(addr_ptr, method_name);
}

auto opmsg(viua::internals::types::byte* addr_ptr,
           int_op reg,
           int_op method_name) -> viua::internals::types::byte* {
    return insert_two_ri_instruction(addr_ptr, MSG, reg, method_name);
}

auto opinsert(viua::internals::types::byte* addr_ptr,
              int_op rega,
              int_op regb,
              int_op regr) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, INSERT, rega, regb, regr);
}

auto opremove(viua::internals::types::byte* addr_ptr,
              int_op rega,
              int_op regb,
              int_op regr) -> viua::internals::types::byte* {
    return insert_three_ri_instruction(addr_ptr, REMOVE, rega, regb, regr);
}

auto opreturn(viua::internals::types::byte* addr_ptr)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = RETURN;
    return addr_ptr;
}

auto ophalt(viua::internals::types::byte* addr_ptr)
    -> viua::internals::types::byte* {
    *(addr_ptr++) = HALT;
    return addr_ptr;
}
}}  // namespace cg::bytecode
