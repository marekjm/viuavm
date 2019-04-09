/*
 *  Copyright (C) 2015-2019 Marek Marecki
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

#include <iostream>
#include <map>
#include <sstream>
#include <viua/bytecode/maps.h>
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/operand_types.h>
#include <viua/cg/disassembler/disassembler.h>
#include <viua/support/env.h>
#include <viua/support/pointer.h>
#include <viua/support/string.h>
#include <viua/util/memory.h>
using namespace std;

using viua::util::memory::load_aligned;


auto disassembler::intop(viua::internals::types::byte* ptr) -> std::string {
    auto oss = ostringstream{};

    auto const type = *reinterpret_cast<OperandType*>(ptr);
    viua::support::pointer::inc<OperandType, viua::internals::types::byte>(ptr);

    if (type == OT_VOID) {
        oss << "void";
    } else if (type == OT_REGISTER_INDEX) {
        oss << '%' << load_aligned<viua::internals::types::register_index>(ptr);
        viua::support::pointer::inc<viua::internals::types::register_index,
                                    viua::internals::types::byte>(ptr);
        viua::support::pointer::inc<viua::internals::Register_sets,
                                    viua::internals::types::byte>(ptr);
    } else if (type == OT_REGISTER_REFERENCE) {
        oss << '@' << load_aligned<viua::internals::types::register_index>(ptr);
        viua::support::pointer::inc<viua::internals::types::register_index,
                                    viua::internals::types::byte>(ptr);
        viua::support::pointer::inc<viua::internals::Register_sets,
                                    viua::internals::types::byte>(ptr);
    } else if (type == OT_POINTER) {
        oss << '*' << load_aligned<viua::internals::types::register_index>(ptr);
        viua::support::pointer::inc<viua::internals::types::register_index,
                                    viua::internals::types::byte>(ptr);
        viua::support::pointer::inc<viua::internals::Register_sets,
                                    viua::internals::types::byte>(ptr);
    } else if (type == OT_INT) {
        oss << load_aligned<viua::internals::types::plain_int>(ptr);
        viua::support::pointer::inc<viua::internals::types::plain_int,
                                    viua::internals::types::byte>(ptr);
    } else {
        // FIXME Throw a real exception.
        throw "invalid operand type detected";
    }

    return oss.str();
}
auto disassembler::intop_with_rs_type(viua::internals::types::byte* ptr)
    -> std::string {
    auto oss = ostringstream{};

    auto const type = *reinterpret_cast<OperandType*>(ptr);
    viua::support::pointer::inc<OperandType, viua::internals::types::byte>(ptr);

    if (type == OT_VOID) {
        oss << "void";
    } else if (type == OT_REGISTER_INDEX) {
        oss << '%' << load_aligned<viua::internals::types::register_index>(ptr);
        viua::support::pointer::inc<viua::internals::types::register_index,
                                    viua::internals::types::byte>(ptr);
        oss << ' ';
        switch (*reinterpret_cast<viua::internals::Register_sets*>(ptr)) {
        case viua::internals::Register_sets::GLOBAL:
            oss << "global";
            break;
        case viua::internals::Register_sets::LOCAL:
            oss << "local";
            break;
        case viua::internals::Register_sets::STATIC:
            oss << "static";
            break;
        case viua::internals::Register_sets::ARGUMENTS:
            oss << "arguments";
            break;
        case viua::internals::Register_sets::PARAMETERS:
            oss << "parameters";
            break;
        case viua::internals::Register_sets::CLOSURE_LOCAL:
            oss << "closure_local";
            break;
        default:
            if (viua::support::env::get_var("VIUA_DISASM_INVALID_RS_TYPES")
                == "yes") {
                oss << "<invalid=" << static_cast<int>(*ptr) << '>';
            } else {
                throw "invalid register set detected";
            }
        }
        viua::support::pointer::inc<viua::internals::Register_sets,
                                    viua::internals::types::byte>(ptr);
    } else if (type == OT_REGISTER_REFERENCE) {
        oss << '@' << load_aligned<viua::internals::types::register_index>(ptr);
        viua::support::pointer::inc<viua::internals::types::register_index,
                                    viua::internals::types::byte>(ptr);
        oss << ' ';
        switch (*reinterpret_cast<viua::internals::Register_sets*>(ptr)) {
        case viua::internals::Register_sets::GLOBAL:
            oss << "global";
            break;
        case viua::internals::Register_sets::LOCAL:
            oss << "local";
            break;
        case viua::internals::Register_sets::STATIC:
            oss << "static";
            break;
        case viua::internals::Register_sets::ARGUMENTS:
            oss << "arguments";
            break;
        case viua::internals::Register_sets::PARAMETERS:
            oss << "parameters";
            break;
        case viua::internals::Register_sets::CLOSURE_LOCAL:
            oss << "closure_local";
            break;
        default:
            throw "invalid register set detected";
        }
        viua::support::pointer::inc<viua::internals::Register_sets,
                                    viua::internals::types::byte>(ptr);
    } else if (type == OT_POINTER) {
        oss << '*' << load_aligned<viua::internals::types::register_index>(ptr);
        viua::support::pointer::inc<viua::internals::types::register_index,
                                    viua::internals::types::byte>(ptr);
        oss << ' ';
        switch (*reinterpret_cast<viua::internals::Register_sets*>(ptr)) {
        case viua::internals::Register_sets::GLOBAL:
            oss << "global";
            break;
        case viua::internals::Register_sets::LOCAL:
            oss << "local";
            break;
        case viua::internals::Register_sets::STATIC:
            oss << "static";
            break;
        case viua::internals::Register_sets::ARGUMENTS:
            oss << "arguments";
            break;
        case viua::internals::Register_sets::PARAMETERS:
            oss << "parameters";
            break;
        case viua::internals::Register_sets::CLOSURE_LOCAL:
            oss << "closure_local";
            break;
        default:
            throw "invalid register set detected";
        }
        viua::support::pointer::inc<viua::internals::Register_sets,
                                    viua::internals::types::byte>(ptr);
    } else if (type == OT_INT) {
        oss << load_aligned<viua::internals::types::plain_int>(ptr);
        viua::support::pointer::inc<viua::internals::types::plain_int,
                                    viua::internals::types::byte>(ptr);
    } else {
        throw "invalid operand type detected";
    }

    return oss.str();
}

static auto disassemble_bit_string(viua::internals::types::byte* ptr,
                                   viua::internals::types::bits_size const size)
    -> std::string {
    static map<uint8_t, char> const decodings = {
        {
            0b0000,
            '0',
        },
        {
            0b0001,
            '1',
        },
        {
            0b0010,
            '2',
        },
        {
            0b0011,
            '3',
        },
        {
            0b0100,
            '4',
        },
        {
            0b0101,
            '5',
        },
        {
            0b0110,
            '6',
        },
        {
            0b0111,
            '7',
        },
        {
            0b1000,
            '8',
        },
        {
            0b1001,
            '9',
        },
        {
            0b1010,
            'a',
        },
        {
            0b1011,
            'b',
        },
        {
            0b1100,
            'c',
        },
        {
            0b1101,
            'd',
        },
        {
            0b1110,
            'e',
        },
        {
            0b1111,
            'f',
        },
    };

    auto oss = ostringstream{};
    oss << "0x";

    static auto const mask_high = uint8_t{0b00001111};
    static auto const mask_low  = uint8_t{0b11110000};

    for (auto i = std::remove_const_t<decltype(size)>(0); i < size; ++i) {
        auto two_digits = *(ptr + i);

        auto high_digit = ((two_digits & mask_low) >> 4);
        auto low_digit  = (two_digits & mask_high);

        oss << decodings.at(static_cast<uint8_t>(high_digit));
        oss << decodings.at(static_cast<uint8_t>(low_digit));
    }

    return oss.str();
}

static auto decode_timeout(viua::internals::types::byte* ptr)
    -> viua::internals::types::timeout {
    return load_aligned<viua::internals::types::timeout>(ptr);
}
static auto disassemble_ri_operand(ostream& oss,
                                   viua::internals::types::byte* ptr)
    -> viua::internals::types::byte* {
    oss << ' ' << disassembler::intop(ptr);

    switch (*ptr) {
    case OT_REGISTER_INDEX:
    case OT_REGISTER_REFERENCE:
    case OT_POINTER:
        viua::support::pointer::inc<OperandType, viua::internals::types::byte>(
            ptr);
        viua::support::pointer::inc<viua::internals::Register_sets,
                                    viua::internals::types::byte>(ptr);
        viua::support::pointer::inc<viua::internals::types::plain_int,
                                    viua::internals::types::byte>(ptr);
        break;
    case OT_VOID:
        viua::support::pointer::inc<OperandType, viua::internals::types::byte>(
            ptr);
        break;
    case OT_INT:
        viua::support::pointer::inc<OperandType, viua::internals::types::byte>(
            ptr);
        viua::support::pointer::inc<viua::internals::types::plain_int,
                                    viua::internals::types::byte>(ptr);
        break;
    default:
        throw "invalid operand type detected";
    }
    return ptr;
}
static auto disassemble_ri_operand_with_rs_type(
    ostream& oss,
    viua::internals::types::byte* ptr) -> viua::internals::types::byte* {
    oss << ' ' << disassembler::intop_with_rs_type(ptr);

    switch (*ptr) {
    case OT_REGISTER_INDEX:
    case OT_REGISTER_REFERENCE:
    case OT_POINTER:
        viua::support::pointer::inc<OperandType, viua::internals::types::byte>(
            ptr);
        viua::support::pointer::inc<viua::internals::Register_sets,
                                    viua::internals::types::byte>(ptr);
        viua::support::pointer::inc<viua::internals::types::plain_int,
                                    viua::internals::types::byte>(ptr);
        break;
    case OT_VOID:
        viua::support::pointer::inc<OperandType, viua::internals::types::byte>(
            ptr);
        break;
    case OT_INT:
        viua::support::pointer::inc<OperandType, viua::internals::types::byte>(
            ptr);
        viua::support::pointer::inc<viua::internals::types::plain_int,
                                    viua::internals::types::byte>(ptr);
        break;
    default:
        throw "invalid operand type detected";
    }
    return ptr;
}
auto disassembler::instruction(viua::internals::types::byte* ptr)
    -> tuple<std::string, viua::internals::types::bytecode_size> {
    viua::internals::types::byte* saved_ptr = ptr;

    auto const op = OPCODE(*saved_ptr);
    auto opname   = std::string{};
    try {
        if (op == PAMV) {
            /*
             * PAMV is an internal instruction, not visible to the
             * user code.
             */
            opname = "move";
        } else if (op == PARAM) {
            /*
             * PARAM is an internal instruction, not visible to the
             * user code.
             */
            opname = "copy";
        } else if (op == ARG) {
            /*
             * ARG is an internal instruction, not visible to the
             * user code.
             */
            opname = "move";
        } else {
            opname = OP_NAMES.at(op);
        }
        ++ptr;
    } catch (std::out_of_range const& e) {
        auto emsg = ostringstream{};
        emsg << "could not find name for opcode: " << op;
        throw emsg.str();
    }

    auto oss = ostringstream{};
    oss << opname;

    if (op == STRING) {
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        ++ptr;  // for operand type
        auto const s = std::string{reinterpret_cast<char*>(ptr)};
        oss << ' ' << str::enquote(s);
        ptr += s.size();
        ++ptr;  // for null character terminating the C-style string not
                // included in std::string
    } else if (op == TEXT) {
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        if (OperandType(*ptr) == OT_REGISTER_INDEX
            or OperandType(*ptr) == OT_POINTER) {
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        } else {
            ++ptr;  // for operand type
            auto const s = std::string{reinterpret_cast<char*>(ptr)};
            oss << ' ' << str::enquote(s);
            ptr += s.size();
            ++ptr;  // for null character terminating the C-style string not
                    // included in std::string
        }
    } else if (op == ATOM) {
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        auto const s = std::string{reinterpret_cast<char*>(ptr)};
        oss << ' ' << str::enquote(s, '\'');
        ptr += s.size();
        ++ptr;  // for null character terminating the C-style string not
                // included in std::string
    } else if ((op == CLOSURE) or (op == FUNCTION)) {
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        oss << ' ';
        auto const fn_name = std::string{reinterpret_cast<char*>(ptr)};
        oss << fn_name;
        ptr += fn_name.size();
        ++ptr;  // for null character terminating the C-style string not
                // included in std::string
    } else if ((op == CALL) or (op == PROCESS)) {
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        oss << ' ';

        if (OperandType(*ptr) == OT_REGISTER_INDEX
            or OperandType(*ptr) == OT_POINTER) {
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        } else {
            auto const fn_name = std::string{reinterpret_cast<char*>(ptr)};
            oss << fn_name;
            ptr += fn_name.size();
            ++ptr;  // for null character terminating the C-style string not
                    // included in std::string
        }
    } else if (op == TAILCALL or op == DEFER) {
        oss << ' ';

        if (OperandType(*ptr) == OT_REGISTER_INDEX
            or OperandType(*ptr) == OT_POINTER) {
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        } else {
            auto const fn_name = std::string{reinterpret_cast<char*>(ptr)};
            oss << fn_name;
            ptr += fn_name.size();
            ++ptr;  // for null character terminating the C-style string not
                    // included in std::string
        }
    } else if ((op == IMPORT) or (op == ENTER) or (op == WATCHDOG)) {
        oss << ' ';
        auto const s = std::string{reinterpret_cast<char*>(ptr)};
        oss << s;
        ptr += s.size();
        ++ptr;  // for null character terminating the C-style string not
                // included in std::string
    } else if (op == CATCH) {
        oss << ' ';
        auto const type_name = std::string{reinterpret_cast<char*>(ptr)};
        oss << str::enquote(type_name);
        ptr += type_name.size();
        ++ptr;  // for null character terminating the C-style string not
                // included in std::string

        oss << ' ';
        auto const block_name = std::string{reinterpret_cast<char*>(ptr)};
        oss << block_name;
        ptr += block_name.size();
        ++ptr;  // for null character terminating the C-style string not
                // included in std::string
    }

    switch (op) {
    case NOP:
    case TRY:
    case LEAVE:
    case RETURN:
    case HALT:
        // These ops have no operands.
        break;
    case STRING:
    case TEXT:
    case ATOM:
    case CLOSURE:
    case FUNCTION:
    case CALL:
    case PROCESS:
    case TAILCALL:
    case DEFER:
    case IMPORT:
    case ENTER:
    case WATCHDOG:
    case CATCH:
        // Already handled in the `if` above.
        break;
    case BITSWIDTH:
    case BITSEQ:
    case BITSLT:
    case BITSLTE:
    case BITSGT:
    case BITSGTE:
    case BITAEQ:
    case BITALT:
    case BITALTE:
    case BITAGT:
    case BITAGTE:
        // Not implemented.
        break;
    case IZERO:
    case PRINT:
    case ECHO:
    case THROW:
    case DRAW:
    case DELETE:
    case IINC:
    case IDEC:
    case SELF:
    case ALLOCATE_REGISTERS:
    case STRUCT:
    case WRAPINCREMENT:
    case WRAPDECREMENT:
    case CHECKEDSINCREMENT:
    case CHECKEDSDECREMENT:
    case CHECKEDUINCREMENT:
    case CHECKEDUDECREMENT:
    case SATURATINGSINCREMENT:
    case SATURATINGSDECREMENT:
    case SATURATINGUINCREMENT:
    case SATURATINGUDECREMENT:
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        break;
    case BOOL:
        ptr = disassemble_ri_operand(oss, ptr);
        break;
    case INTEGER:
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        ptr = disassemble_ri_operand(oss, ptr);

        break;
    case FRAME:
        ptr = disassemble_ri_operand(oss, ptr);
        ptr = disassemble_ri_operand(oss, ptr);
        break;
    case ARG:
    case PARAM:
    case PAMV:
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        break;
    case SEND:
    case ITOF:
    case FTOI:
    case STOI:
    case STOF:
    case ISNULL:
    case NOT:
    case MOVE:
    case COPY:
    case PTR:
    case PTRLIVE:
    case SWAP:
    case VPUSH:
    case VLEN:
    case TEXTLENGTH:
    case STRUCTKEYS:
    case BITNOT:
    case ROL:
    case ROR:
    case BITS_OF_INTEGER:
    case INTEGER_OF_BITS:
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        break;
    case BITS:
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        if (OperandType(*ptr) == OT_BITS) {
            ++ptr;  // for operand type
            auto bsz = load_aligned<viua::internals::types::bits_size>(ptr);
            viua::support::pointer::inc<viua::internals::types::bits_size,
                                        viua::internals::types::byte>(ptr);
            oss << ' ';
            oss << disassemble_bit_string(ptr, bsz);
            ptr += bsz;
        } else {
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        }

        break;
    case ADD:
    case SUB:
    case MUL:
    case DIV:
    case LT:
    case LTE:
    case GT:
    case GTE:
    case EQ:
    case BITAND:
    case BITOR:
    case BITXOR:
    case BITAT:
    case SHL:
    case SHR:
    case ASHL:
    case ASHR:
    case WRAPADD:
    case WRAPSUB:
    case WRAPMUL:
    case WRAPDIV:
    case CHECKEDSADD:
    case CHECKEDSSUB:
    case CHECKEDSMUL:
    case CHECKEDSDIV:
    case CHECKEDUADD:
    case CHECKEDUSUB:
    case CHECKEDUMUL:
    case CHECKEDUDIV:
    case SATURATINGSADD:
    case SATURATINGSSUB:
    case SATURATINGSMUL:
    case SATURATINGSDIV:
    case SATURATINGUADD:
    case SATURATINGUSUB:
    case SATURATINGUMUL:
    case SATURATINGUDIV:
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        break;
    case BITSET:
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        if (OperandType(*ptr) == OT_TRUE) {
            oss << ' ' << "true";
            ++ptr;
        } else if (OperandType(*ptr) == OT_FALSE) {
            oss << ' ' << "false";
            ++ptr;
        } else {
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        }

        break;
    case VECTOR:
    case VAT:
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        ptr = disassemble_ri_operand(oss, ptr);

        break;
    case CAPTURE:
    case CAPTURECOPY:
    case CAPTUREMOVE:
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        ptr = disassemble_ri_operand(oss, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        break;
    case AND:
    case OR:
    case TEXTEQ:
    case TEXTAT:
    case TEXTCOMMONPREFIX:
    case TEXTCOMMONSUFFIX:
    case TEXTCONCAT:
    case VINSERT:
    case VPOP:
    case ATOMEQ:
    case PIDEQ:
    case STRUCTINSERT:
    case STRUCTREMOVE:
    case STRUCTAT:
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        break;
    case TEXTSUB:
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        break;
    case STREQ:
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        break;
    case JUMP:
        oss << " 0x";
        oss << hex;
        oss << load_aligned<uint64_t>(ptr);  // FIXME use Viua-defined type
        viua::support::pointer::inc<uint64_t, viua::internals::types::byte>(
            ptr);

        oss << dec;

        break;
    case IF:
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        oss << " 0x";
        oss << hex;
        oss << load_aligned<uint64_t>(ptr);  // FIXME use Viua-defined type
        viua::support::pointer::inc<uint64_t, viua::internals::types::byte>(
            ptr);

        oss << " 0x";
        oss << hex;
        oss << load_aligned<uint64_t>(ptr);  // FIXME use Viua-defined type
        viua::support::pointer::inc<uint64_t, viua::internals::types::byte>(
            ptr);

        oss << dec;

        break;
    case FLOAT:
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        oss << ' ';
        oss << load_aligned<viua::internals::types::plain_float>(ptr);
        viua::support::pointer::inc<viua::internals::types::plain_float,
                                    viua::internals::types::byte>(ptr);
        break;
    case JOIN:
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        viua::support::pointer::inc<viua::internals::types::byte,
                                    viua::internals::types::byte>(ptr);
        oss << ' ';
        if (decode_timeout(ptr)) {
            oss << decode_timeout(ptr) - 1 << "ms";
        } else {
            oss << "infinity";
        }
        viua::support::pointer::inc<viua::internals::types::timeout,
                                    viua::internals::types::byte>(ptr);

        break;
    case RECEIVE:
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        viua::support::pointer::inc<viua::internals::types::byte,
                                    viua::internals::types::byte>(ptr);
        oss << ' ';
        if (decode_timeout(ptr)) {
            oss << decode_timeout(ptr) - 1 << "ms";
        } else {
            oss << "infinity";
        }
        viua::support::pointer::inc<viua::internals::types::timeout,
                                    viua::internals::types::byte>(ptr);

        break;
    default:
        // if opcode was not covered here, it means it must have been a
        // variable-length opcode
        oss << "";
    }

    if (ptr < saved_ptr) {
        throw("bytecode pointer increase less than zero: near "
              + OP_NAMES.at(op) + " instruction");
    }
    // we already *know* that the result will not be negative
    auto const increase =
        static_cast<viua::internals::types::bytecode_size>(ptr - saved_ptr);

    return tuple<std::string, viua::internals::types::bytecode_size>(oss.str(),
                                                                     increase);
}
