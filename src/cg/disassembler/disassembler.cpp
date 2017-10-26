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


string disassembler::intop(viua::internals::types::byte* ptr) {
    ostringstream oss;

    auto type = *reinterpret_cast<OperandType*>(ptr);
    pointer::inc<OperandType, viua::internals::types::byte>(ptr);

    switch (type) {
        case OT_VOID:
            oss << "void";
            break;
        case OT_REGISTER_INDEX:
            oss << '%' << load_aligned<viua::internals::types::register_index>(ptr);
            pointer::inc<viua::internals::types::register_index, viua::internals::types::byte>(ptr);
            pointer::inc<viua::internals::RegisterSets, viua::internals::types::byte>(ptr);
            break;
        case OT_REGISTER_REFERENCE:
            oss << '@' << load_aligned<viua::internals::types::register_index>(ptr);
            pointer::inc<viua::internals::types::register_index, viua::internals::types::byte>(ptr);
            pointer::inc<viua::internals::RegisterSets, viua::internals::types::byte>(ptr);
            break;
        case OT_POINTER:
            oss << '*' << load_aligned<viua::internals::types::register_index>(ptr);
            pointer::inc<viua::internals::types::register_index, viua::internals::types::byte>(ptr);
            pointer::inc<viua::internals::RegisterSets, viua::internals::types::byte>(ptr);
            break;
        case OT_INT:
            oss << load_aligned<viua::internals::types::plain_int>(ptr);
            pointer::inc<viua::internals::types::plain_int, viua::internals::types::byte>(ptr);
            break;
        default:
            throw "invalid operand type detected";
    }

    return oss.str();
}
string disassembler::intop_with_rs_type(viua::internals::types::byte* ptr) {
    ostringstream oss;

    auto type = *reinterpret_cast<OperandType*>(ptr);
    pointer::inc<OperandType, viua::internals::types::byte>(ptr);

    switch (type) {
        case OT_VOID:
            oss << "void";
            break;
        case OT_REGISTER_INDEX:
            oss << '%' << load_aligned<viua::internals::types::register_index>(ptr);
            pointer::inc<viua::internals::types::register_index, viua::internals::types::byte>(ptr);
            oss << ' ';
            switch (*reinterpret_cast<viua::internals::RegisterSets*>(ptr)) {
                case viua::internals::RegisterSets::CURRENT:
                    oss << "current";
                    break;
                case viua::internals::RegisterSets::GLOBAL:
                    oss << "global";
                    break;
                case viua::internals::RegisterSets::LOCAL:
                    oss << "local";
                    break;
                case viua::internals::RegisterSets::STATIC:
                    oss << "static";
                    break;
                default:
                    if (support::env::getvar("VIUA_DISASM_INVALID_RS_TYPES") == "yes") {
                        oss << "<invalid=" << int(*ptr) << '>';
                    } else {
                        throw "invalid register set detected";
                    }
            }
            pointer::inc<viua::internals::RegisterSets, viua::internals::types::byte>(ptr);
            break;
        case OT_REGISTER_REFERENCE:
            oss << '@' << load_aligned<viua::internals::types::register_index>(ptr);
            pointer::inc<viua::internals::types::register_index, viua::internals::types::byte>(ptr);
            oss << ' ';
            switch (*reinterpret_cast<viua::internals::RegisterSets*>(ptr)) {
                case viua::internals::RegisterSets::CURRENT:
                    oss << "current";
                    break;
                case viua::internals::RegisterSets::GLOBAL:
                    oss << "global";
                    break;
                case viua::internals::RegisterSets::LOCAL:
                    oss << "local";
                    break;
                case viua::internals::RegisterSets::STATIC:
                    oss << "static";
                    break;
                default:
                    throw "invalid register set detected";
            }
            pointer::inc<viua::internals::RegisterSets, viua::internals::types::byte>(ptr);
            break;
        case OT_POINTER:
            oss << '*' << load_aligned<viua::internals::types::register_index>(ptr);
            pointer::inc<viua::internals::types::register_index, viua::internals::types::byte>(ptr);
            oss << ' ';
            switch (*reinterpret_cast<viua::internals::RegisterSets*>(ptr)) {
                case viua::internals::RegisterSets::CURRENT:
                    oss << "current";
                    break;
                case viua::internals::RegisterSets::GLOBAL:
                    oss << "global";
                    break;
                case viua::internals::RegisterSets::LOCAL:
                    oss << "local";
                    break;
                case viua::internals::RegisterSets::STATIC:
                    oss << "static";
                    break;
                default:
                    throw "invalid register set detected";
            }
            pointer::inc<viua::internals::RegisterSets, viua::internals::types::byte>(ptr);
            break;
        case OT_INT:
            oss << load_aligned<viua::internals::types::plain_int>(ptr);
            pointer::inc<viua::internals::types::plain_int, viua::internals::types::byte>(ptr);
            break;
        default:
            throw "invalid operand type detected";
    }

    return oss.str();
}
static auto disassemble_bit_string(viua::internals::types::byte* ptr,
                                   const viua::internals::types::bits_size size) -> string {
    const static map<uint8_t, char> decodings = {
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

    ostringstream oss;
    oss << "0x";

    const static uint8_t mask_high = 0b00001111;
    const static uint8_t mask_low = 0b11110000;

    for (std::remove_const_t<decltype(size)> i = 0; i < size; ++i) {
        auto two_digits = *(ptr + i);

        auto high_digit = ((two_digits & mask_low) >> 4);
        auto low_digit = (two_digits & mask_high);

        oss << decodings.at(static_cast<uint8_t>(high_digit));
        oss << decodings.at(static_cast<uint8_t>(low_digit));
    }

    return oss.str();
}

static viua::internals::types::timeout decode_timeout(viua::internals::types::byte* ptr) {
    return load_aligned<viua::internals::types::timeout>(ptr);
}
static viua::internals::types::byte* disassemble_ri_operand(ostream& oss, viua::internals::types::byte* ptr) {
    oss << ' ' << disassembler::intop(ptr);

    switch (*ptr) {
        case OT_REGISTER_INDEX:
        case OT_REGISTER_REFERENCE:
        case OT_POINTER:
            pointer::inc<OperandType, viua::internals::types::byte>(ptr);
            pointer::inc<viua::internals::RegisterSets, viua::internals::types::byte>(ptr);
            pointer::inc<viua::internals::types::plain_int, viua::internals::types::byte>(ptr);
            break;
        case OT_VOID:
            pointer::inc<OperandType, viua::internals::types::byte>(ptr);
            break;
        case OT_INT:
            pointer::inc<OperandType, viua::internals::types::byte>(ptr);
            pointer::inc<viua::internals::types::plain_int, viua::internals::types::byte>(ptr);
            break;
        default:
            throw "invalid operand type detected";
    }
    return ptr;
}
static viua::internals::types::byte* disassemble_ri_operand_with_rs_type(ostream& oss,
                                                                         viua::internals::types::byte* ptr) {
    oss << ' ' << disassembler::intop_with_rs_type(ptr);

    switch (*ptr) {
        case OT_REGISTER_INDEX:
        case OT_REGISTER_REFERENCE:
        case OT_POINTER:
            pointer::inc<OperandType, viua::internals::types::byte>(ptr);
            pointer::inc<viua::internals::RegisterSets, viua::internals::types::byte>(ptr);
            pointer::inc<viua::internals::types::plain_int, viua::internals::types::byte>(ptr);
            break;
        case OT_VOID:
            pointer::inc<OperandType, viua::internals::types::byte>(ptr);
            break;
        case OT_INT:
            pointer::inc<OperandType, viua::internals::types::byte>(ptr);
            pointer::inc<viua::internals::types::plain_int, viua::internals::types::byte>(ptr);
            break;
        default:
            throw "invalid operand type detected";
    }
    return ptr;
}
tuple<string, viua::internals::types::bytecode_size> disassembler::instruction(
    viua::internals::types::byte* ptr) {
    viua::internals::types::byte* saved_ptr = ptr;

    OPCODE op = OPCODE(*saved_ptr);
    string opname;
    try {
        opname = OP_NAMES.at(op);
        ++ptr;
    } catch (const std::out_of_range& e) {
        ostringstream emsg;
        emsg << "could not find name for opcode: " << op;
        throw emsg.str();
    }

    ostringstream oss;
    oss << opname;

    if (op == STRSTORE) {
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        ++ptr;  // for operand type
        string s = string(reinterpret_cast<char*>(ptr));
        oss << ' ' << str::enquote(s);
        ptr += s.size();
        ++ptr;  // for null character terminating the C-style string not included in std::string
    } else if (op == TEXT) {
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        if (OperandType(*ptr) == OT_REGISTER_INDEX or OperandType(*ptr) == OT_POINTER) {
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        } else {
            ++ptr;  // for operand type
            string s = string(reinterpret_cast<char*>(ptr));
            oss << ' ' << str::enquote(s);
            ptr += s.size();
            ++ptr;  // for null character terminating the C-style string not included in std::string
        }
    } else if (op == ATOM) {
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        string s = string(reinterpret_cast<char*>(ptr));
        oss << ' ' << str::enquote(s, '\'');
        ptr += s.size();
        ++ptr;  // for null character terminating the C-style string not included in std::string
    } else if ((op == CLOSURE) or (op == FUNCTION)) {
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        oss << ' ';
        string fn_name = string(reinterpret_cast<char*>(ptr));
        oss << fn_name;
        ptr += fn_name.size();
        ++ptr;  // for null character terminating the C-style string not included in std::string
    } else if ((op == CALL) or (op == PROCESS) or (op == MSG)) {
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        oss << ' ';

        if (OperandType(*ptr) == OT_REGISTER_INDEX or OperandType(*ptr) == OT_POINTER) {
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        } else {
            string fn_name = string(reinterpret_cast<char*>(ptr));
            oss << fn_name;
            ptr += fn_name.size();
            ++ptr;  // for null character terminating the C-style string not included in std::string
        }
    } else if ((op == CLASS) or (op == NEW) or (op == DERIVE)) {
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        oss << ' ';
        string fn_name = string(reinterpret_cast<char*>(ptr));
        oss << fn_name;
        ptr += fn_name.size();
        ++ptr;  // for null character terminating the C-style string not included in std::string
    } else if (op == TAILCALL or op == DEFER) {
        oss << ' ';

        if (OperandType(*ptr) == OT_REGISTER_INDEX or OperandType(*ptr) == OT_POINTER) {
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
        } else {
            string fn_name = string(reinterpret_cast<char*>(ptr));
            oss << fn_name;
            ptr += fn_name.size();
            ++ptr;  // for null character terminating the C-style string not included in std::string
        }
    } else if ((op == IMPORT) or (op == ENTER) or (op == WATCHDOG)) {
        oss << ' ';
        string s = string(reinterpret_cast<char*>(ptr));
        oss << (op == IMPORT ? str::enquote(s) : s);
        ptr += s.size();
        ++ptr;  // for null character terminating the C-style string not included in std::string
    } else if (op == CATCH) {
        string s;

        oss << ' ';
        s = string(reinterpret_cast<char*>(ptr));
        oss << str::enquote(s);
        ptr += s.size();
        ++ptr;  // for null character terminating the C-style string not included in std::string

        oss << ' ';
        s = string(reinterpret_cast<char*>(ptr));
        oss << s;
        ptr += s.size();
        ++ptr;  // for null character terminating the C-style string not included in std::string
    } else if (op == ATTACH) {
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        oss << ' ';
        string fn_name = string(reinterpret_cast<char*>(ptr));
        oss << fn_name;
        ptr += fn_name.size();
        ++ptr;  // for null character terminating the C-style string not included in std::string

        oss << ' ';
        string md_name = string(reinterpret_cast<char*>(ptr));
        oss << md_name;
        ptr += md_name.size();
        ++ptr;  // for null character terminating the C-style string not included in std::string
    }

    switch (op) {
        case IZERO:
        case PRINT:
        case ECHO:
        case THROW:
        case DRAW:
        case DELETE:
        case IINC:
        case IDEC:
        case SELF:
        case ARGC:
        case STRUCT:
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
            break;
        case BOOL:
        case REGISTER:
            ptr = disassemble_ri_operand(oss, ptr);
            break;
        case ISTORE:
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
            ptr = disassemble_ri_operand(oss, ptr);

            break;
        case FRAME:
            ptr = disassemble_ri_operand(oss, ptr);
            ptr = disassemble_ri_operand(oss, ptr);
            break;
        case ARG:
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
            ptr = disassemble_ri_operand(oss, ptr);
            break;
        case PARAM:
        case PAMV:
            ptr = disassemble_ri_operand(oss, ptr);
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
        case SWAP:
        case VPUSH:
        case VLEN:
        case TEXTLENGTH:
        case STRUCTKEYS:
        case BITNOT:
        case ROL:
        case ROR:
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

            break;
        case BITS:
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

            if (OperandType(*ptr) == OT_BITS) {
                ++ptr;  // for operand type
                auto bsz = load_aligned<viua::internals::types::bits_size>(ptr);
                pointer::inc<viua::internals::types::bits_size, viua::internals::types::byte>(ptr);
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
        case VEC:
        case VINSERT:
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
        case VPOP:
        case ATOMEQ:
        case STRUCTINSERT:
        case STRUCTREMOVE:
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
        case INSERT:
        case REMOVE:
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

            break;
        case JUMP:
            oss << " 0x";
            oss << hex;
            oss << load_aligned<uint64_t>(ptr);  // FIXME use Viua-defined type
            pointer::inc<uint64_t, viua::internals::types::byte>(ptr);

            oss << dec;

            break;
        case IF:
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

            oss << " 0x";
            oss << hex;
            oss << load_aligned<uint64_t>(ptr);  // FIXME use Viua-defined type
            pointer::inc<uint64_t, viua::internals::types::byte>(ptr);

            oss << " 0x";
            oss << hex;
            oss << load_aligned<uint64_t>(ptr);  // FIXME use Viua-defined type
            pointer::inc<uint64_t, viua::internals::types::byte>(ptr);

            oss << dec;

            break;
        case FSTORE:
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

            oss << ' ';
            oss << load_aligned<viua::internals::types::plain_float>(ptr);
            pointer::inc<viua::internals::types::plain_float, viua::internals::types::byte>(ptr);
            break;
        case RESS:
            oss << ' ';
            switch (static_cast<viua::internals::RegisterSets>(*ptr)) {
                case viua::internals::RegisterSets::GLOBAL:
                    oss << "global";
                    break;
                case viua::internals::RegisterSets::LOCAL:
                    oss << "local";
                    break;
                case viua::internals::RegisterSets::STATIC:
                    oss << "static";
                    break;
                default:
                    // FIXME: should this only be a warning?
                    oss << "; WARNING: invalid register set type\n";
                    oss << int(*ptr);
            }
            pointer::inc<viua::internals::types::registerset_type_marker, viua::internals::types::byte>(ptr);
            break;
        case JOIN:
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

            pointer::inc<viua::internals::types::byte, viua::internals::types::byte>(ptr);
            oss << ' ';
            if (decode_timeout(ptr)) {
                oss << decode_timeout(ptr) - 1 << "ms";
            } else {
                oss << "infinity";
            }
            pointer::inc<viua::internals::types::timeout, viua::internals::types::byte>(ptr);

            break;
        case RECEIVE:
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

            pointer::inc<viua::internals::types::byte, viua::internals::types::byte>(ptr);
            oss << ' ';
            if (decode_timeout(ptr)) {
                oss << decode_timeout(ptr) - 1 << "ms";
            } else {
                oss << "infinity";
            }
            pointer::inc<viua::internals::types::timeout, viua::internals::types::byte>(ptr);

            break;
        default:
            // if opcode was not covered here, it means it must have been a variable-length opcode
            oss << "";
    }

    if (ptr < saved_ptr) {
        throw("bytecode pointer increase less than zero: near " + OP_NAMES.at(op) + " instruction");
    }
    // we already *know* that the result will not be negative
    auto increase = static_cast<viua::internals::types::bytecode_size>(ptr - saved_ptr);

    return tuple<string, viua::internals::types::bytecode_size>(oss.str(), increase);
}
