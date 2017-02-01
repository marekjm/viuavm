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

#include <iostream>
#include <sstream>
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/maps.h>
#include <viua/support/string.h>
#include <viua/support/pointer.h>
#include <viua/cg/disassembler/disassembler.h>
using namespace std;


string disassembler::intop(viua::internals::types::byte* ptr) {
    ostringstream oss;

    auto type = *reinterpret_cast<OperandType*>(ptr);
    pointer::inc<OperandType, viua::internals::types::byte>(ptr);

    switch (type) {
        case OT_VOID:
            oss << "void";
            break;
        case OT_REGISTER_INDEX:
            oss << '%' << *reinterpret_cast<viua::internals::types::register_index*>(ptr);
            pointer::inc<viua::internals::types::register_index, viua::internals::types::byte>(ptr);
            pointer::inc<viua::internals::RegisterSets, viua::internals::types::byte>(ptr);
            break;
        case OT_REGISTER_REFERENCE:
            oss << '@' << *reinterpret_cast<viua::internals::types::register_index*>(ptr);
            pointer::inc<viua::internals::types::register_index, viua::internals::types::byte>(ptr);
            pointer::inc<viua::internals::RegisterSets, viua::internals::types::byte>(ptr);
            break;
        case OT_POINTER:
            oss << '*' << *reinterpret_cast<viua::internals::types::register_index*>(ptr);
            pointer::inc<viua::internals::types::register_index, viua::internals::types::byte>(ptr);
            pointer::inc<viua::internals::RegisterSets, viua::internals::types::byte>(ptr);
            break;
        case OT_INT:
            oss << *reinterpret_cast<viua::internals::types::plain_int*>(ptr);
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
            oss << '%' << *reinterpret_cast<viua::internals::types::register_index*>(ptr);
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
        case OT_REGISTER_REFERENCE:
            oss << '@' << *reinterpret_cast<viua::internals::types::register_index*>(ptr);
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
            oss << '*' << *reinterpret_cast<viua::internals::types::register_index*>(ptr);
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
            oss << *reinterpret_cast<viua::internals::types::plain_int*>(ptr);
            pointer::inc<viua::internals::types::plain_int, viua::internals::types::byte>(ptr);
            break;
        default:
            throw "invalid operand type detected";
    }

    return oss.str();
}

static viua::internals::types::timeout decode_timeout(viua::internals::types::byte *ptr) {
    return *reinterpret_cast<viua::internals::types::timeout*>(ptr);
}
static viua::internals::types::byte* disassemble_ri_operand(ostream& oss, viua::internals::types::byte *ptr) {
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
static viua::internals::types::byte* disassemble_ri_operand_with_rs_type(ostream& oss, viua::internals::types::byte *ptr) {
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
tuple<string, viua::internals::types::bytecode_size> disassembler::instruction(viua::internals::types::byte* ptr) {
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

        string s = string(reinterpret_cast<char*>(ptr));
        oss << ' ' << str::enquote(s);
        ptr += s.size();
        ++ptr; // for null character terminating the C-style string not included in std::string
    } else if ((op == CLOSURE) or (op == FUNCTION)) {
        ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

        oss << ' ';
        string fn_name = string(reinterpret_cast<char*>(ptr));
        oss << fn_name;
        ptr += fn_name.size();
        ++ptr; // for null character terminating the C-style string not included in std::string
    } else if ((op == CALL) or (op == PROCESS) or (op == CLASS) or (op == NEW) or (op == DERIVE) or (op == MSG)) {
        ptr = disassemble_ri_operand(oss, ptr);

        oss << ' ';
        string fn_name = string(reinterpret_cast<char*>(ptr));
        oss << fn_name;
        ptr += fn_name.size();
        ++ptr; // for null character terminating the C-style string not included in std::string
    } else if ((op == IMPORT) or (op == ENTER) or (op == LINK) or (op == WATCHDOG) or (op == TAILCALL)) {
        oss << ' ';
        string s = string(reinterpret_cast<char*>(ptr));
        oss << (op == IMPORT ? str::enquote(s) : s);
        ptr += s.size();
        ++ptr; // for null character terminating the C-style string not included in std::string
    } else if (op == CATCH) {
        string s;

        oss << ' ';
        s = string(reinterpret_cast<char*>(ptr));
        oss << str::enquote(s);
        ptr += s.size();
        ++ptr; // for null character terminating the C-style string not included in std::string

        oss << ' ';
        s = string(reinterpret_cast<char*>(ptr));
        oss << s;
        ptr += s.size();
        ++ptr; // for null character terminating the C-style string not included in std::string
    } else if (op == ATTACH) {
        ptr = disassemble_ri_operand(oss, ptr);

        oss << ' ';
        string fn_name = string(reinterpret_cast<char*>(ptr));
        oss << fn_name;
        ptr += fn_name.size();
        ++ptr; // for null character terminating the C-style string not included in std::string

        oss << ' ';
        string md_name = string(reinterpret_cast<char*>(ptr));
        oss << md_name;
        ptr += md_name.size();
        ++ptr; // for null character terminating the C-style string not included in std::string
    }

    switch (op) {
        case IZERO:
        case PRINT:
        case ECHO:
        case THROW:
        case DELETE:
        case IINC:
        case IDEC:
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
            break;
        case BOOL:
        case ARGC:
        case SELF:
        case DRAW:
        case REGISTER:
            ptr = disassemble_ri_operand(oss, ptr);
            break;
        case ISTORE:
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
            ptr = disassemble_ri_operand(oss, ptr);

            break;
        case ITOF:
        case FTOI:
        case STOI:
        case STOF:
        case ISNULL:
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

            break;
        case SEND:
        case FRAME:
        case ARG:
        case PARAM:
        case PAMV:
        case FCALL:
            ptr = disassemble_ri_operand(oss, ptr);
            ptr = disassemble_ri_operand(oss, ptr);
            break;
        case NOT:
        case MOVE:
        case COPY:
        case PTR:
        case SWAP:
        case VPUSH:
        case VLEN:
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

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
            if (*ptr == OperandType::OT_INT) {
                oss << ' ' << "int";
            } else if (*ptr == OperandType::OT_INT8) {
                oss << ' ' << "int8";
            } else if (*ptr == OperandType::OT_INT16) {
                oss << ' ' << "int16";
            } else if (*ptr == OperandType::OT_INT32) {
                oss << ' ' << "int32";
            } else if (*ptr == OperandType::OT_INT64) {
                oss << ' ' << "int64";
            } else if (*ptr == OperandType::OT_UINT) {
                oss << ' ' << "uint";
            } else if (*ptr == OperandType::OT_UINT8) {
                oss << ' ' << "uint8";
            } else if (*ptr == OperandType::OT_UINT16) {
                oss << ' ' << "uint16";
            } else if (*ptr == OperandType::OT_UINT32) {
                oss << ' ' << "uint32";
            } else if (*ptr == OperandType::OT_UINT64) {
                oss << ' ' << "uint64";
            } else if (*ptr == OperandType::OT_FLOAT) {
                oss << ' ' << "float";
            } else if (*ptr == OperandType::OT_FLOAT32) {
                oss << ' ' << "float32";
            } else if (*ptr == OperandType::OT_FLOAT64) {
                oss << ' ' << "float64";
            } else {
                oss << ' ' << "void";
            }
            ++ptr;
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

            break;
        case VEC:
        case VINSERT:
        case VAT:
        case VPOP:
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
        case STREQ:
        case AND:
        case OR:
        case INSERT:
        case REMOVE:
            ptr = disassemble_ri_operand(oss, ptr);
            ptr = disassemble_ri_operand(oss, ptr);
            ptr = disassemble_ri_operand(oss, ptr);

            break;
        case JUMP:
            oss << " 0x";
            oss << hex;
            oss << *reinterpret_cast<uint64_t*>(ptr);
            pointer::inc<uint64_t, viua::internals::types::byte>(ptr);

            oss << dec;

            break;
        case IF:
            ptr = disassemble_ri_operand(oss, ptr);

            oss << " 0x";
            oss << hex;
            oss << *reinterpret_cast<uint64_t*>(ptr);
            pointer::inc<uint64_t, viua::internals::types::byte>(ptr);

            oss << " 0x";
            oss << hex;
            oss << *reinterpret_cast<uint64_t*>(ptr);
            pointer::inc<uint64_t, viua::internals::types::byte>(ptr);

            oss << dec;

            break;
        case FSTORE:
            ptr = disassemble_ri_operand_with_rs_type(oss, ptr);

            oss << ' ';
            oss << *reinterpret_cast<viua::internals::types::plain_float*>(ptr);
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
            ptr = disassemble_ri_operand(oss, ptr);
        case RECEIVE:
            ptr = disassemble_ri_operand(oss, ptr);

            pointer::inc<viua::internals::types::byte, viua::internals::types::byte>(ptr);
            oss << ' ';
            if (decode_timeout(ptr)) {
                oss << decode_timeout(ptr)-1 << "ms";
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
        throw ("bytecode pointer increase less than zero: near " + OP_NAMES.at(op) + " instruction");
    }
    // we already *know* that the result will not be negative
    auto increase = static_cast<viua::internals::types::bytecode_size>(ptr-saved_ptr);

    return tuple<string, viua::internals::types::bytecode_size>(oss.str(), increase);
}
