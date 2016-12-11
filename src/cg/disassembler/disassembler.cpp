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

#include <sstream>
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/maps.h>
#include <viua/support/string.h>
#include <viua/support/pointer.h>
#include <viua/cg/disassembler/disassembler.h>
using namespace std;


string disassembler::intop(byte* ptr) {
    ostringstream oss;

    auto type = *reinterpret_cast<OperandType*>(ptr);
    pointer::inc<OperandType, byte>(ptr);

    switch (type) {
        case OT_VOID:
            oss << "void";
            break;
        case OT_REGISTER_INDEX:
            oss << *reinterpret_cast<viua::internals::types::register_index*>(ptr);
            pointer::inc<viua::internals::types::register_index, byte>(ptr);
            break;
        case OT_REGISTER_REFERENCE:
            oss << '@' << *reinterpret_cast<viua::internals::types::register_index*>(ptr);
            pointer::inc<viua::internals::types::register_index, byte>(ptr);
            break;
        case OT_POINTER:
            oss << '*' << *reinterpret_cast<viua::internals::types::register_index*>(ptr);
            pointer::inc<viua::internals::types::register_index, byte>(ptr);
            break;
        case OT_INT:
            oss << *reinterpret_cast<viua::internals::types::plain_int*>(ptr);
            pointer::inc<viua::internals::types::plain_int, byte>(ptr);
            break;
        default:
            throw "invalid operand type detected";
    }

    return oss.str();
}

static viua::internals::types::plain_int decode_integer(byte *ptr) {
    return *reinterpret_cast<viua::internals::types::plain_int*>(ptr);
}
static viua::internals::types::timeout decode_timeout(byte *ptr) {
    return *reinterpret_cast<viua::internals::types::timeout*>(ptr);
}
static byte* disassemble_target_register(ostream& oss, byte *ptr) {
    oss << " " << disassembler::intop(ptr);

    switch (*ptr) {
        case OT_REGISTER_INDEX:
        case OT_REGISTER_REFERENCE:
        case OT_POINTER:
            pointer::inc<OperandType, byte>(ptr);
            pointer::inc<viua::internals::types::plain_int, byte>(ptr);
            break;
        case OT_VOID:
            pointer::inc<OperandType, byte>(ptr);
            break;
        default:
            throw "invalid operand type detected";
    }
    return ptr;
}
tuple<string, unsigned> disassembler::instruction(byte* ptr) {
    byte* saved_ptr = ptr;

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
        ptr = disassemble_target_register(oss, ptr);

        string s = string(reinterpret_cast<char*>(ptr));
        oss << " " << str::enquote(s);
        ptr += s.size();
        ++ptr; // for null character terminating the C-style string not included in std::string
    } else if ((op == CALL) or (op == PROCESS) or (op == CLOSURE) or (op == FUNCTION) or (op == CLASS) or (op == NEW) or (op == DERIVE) or (op == MSG)) {
        ptr = disassemble_target_register(oss, ptr);

        oss << " ";
        string fn_name = string(reinterpret_cast<char*>(ptr));
        oss << fn_name;
        ptr += fn_name.size();
        ++ptr; // for null character terminating the C-style string not included in std::string
    } else if ((op == IMPORT) or (op == ENTER) or (op == LINK) or (op == WATCHDOG) or (op == TAILCALL)) {
        oss << " ";
        string s = string(reinterpret_cast<char*>(ptr));
        oss << (op == IMPORT ? str::enquote(s) : s);
        ptr += s.size();
        ++ptr; // for null character terminating the C-style string not included in std::string
    } else if (op == CATCH) {
        string s;

        oss << " ";
        s = string(reinterpret_cast<char*>(ptr));
        oss << str::enquote(s);
        ptr += s.size();
        ++ptr; // for null character terminating the C-style string not included in std::string

        oss << " ";
        s = string(reinterpret_cast<char*>(ptr));
        oss << s;
        ptr += s.size();
        ++ptr; // for null character terminating the C-style string not included in std::string
    } else if (op == ATTACH) {
        oss << " " << intop(ptr);
        pointer::inc<bool, byte>(ptr);
        pointer::inc<viua::internals::types::register_index, byte>(ptr);

        oss << " ";
        string fn_name = string(reinterpret_cast<char*>(ptr));
        oss << fn_name;
        ptr += fn_name.size();
        ++ptr; // for null character terminating the C-style string not included in std::string

        oss << " ";
        string md_name = string(reinterpret_cast<char*>(ptr));
        oss << md_name;
        ptr += md_name.size();
        ++ptr; // for null character terminating the C-style string not included in std::string
    }

    switch (op) {
        case IZERO:
        case IINC:
        case IDEC:
        case PRINT:
        case ECHO:
        case BOOL:
        case DELETE:
        case TMPRI:
        case TMPRO:
        case ARGC:
        case SELF:
        case THROW:
        case DRAW:
        case REGISTER:
            ptr = disassemble_target_register(oss, ptr);
            break;
        case ISTORE:
            ptr = disassemble_target_register(oss, ptr);

            oss << ' ';
            if (*ptr == OT_REGISTER_REFERENCE) {
                oss << '@';
            }
            pointer::inc<bool, byte>(ptr);
            oss << decode_integer(ptr);
            pointer::inc<viua::internals::types::plain_int, byte>(ptr);

            break;
        case ITOF:
        case FTOI:
        case STOI:
        case STOF:
        case SEND:
        case FRAME:
        case ARG:
        case PARAM:
        case PAMV:
        case NOT:
        case MOVE:
        case COPY:
        case PTR:
        case SWAP:
        case ISNULL:
        case VPUSH:
        case VLEN:
        case FCALL:
            ptr = disassemble_target_register(oss, ptr);

            oss << " " << intop(ptr);
            pointer::inc<bool, byte>(ptr);
            pointer::inc<viua::internals::types::register_index, byte>(ptr);

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
                oss << " " << "int";
            } else if (*ptr == OperandType::OT_INT8) {
                oss << " " << "int8";
            } else if (*ptr == OperandType::OT_INT16) {
                oss << " " << "int16";
            } else if (*ptr == OperandType::OT_INT32) {
                oss << " " << "int32";
            } else if (*ptr == OperandType::OT_INT64) {
                oss << " " << "int64";
            } else if (*ptr == OperandType::OT_UINT) {
                oss << " " << "uint";
            } else if (*ptr == OperandType::OT_UINT8) {
                oss << " " << "uint8";
            } else if (*ptr == OperandType::OT_UINT16) {
                oss << " " << "uint16";
            } else if (*ptr == OperandType::OT_UINT32) {
                oss << " " << "uint32";
            } else if (*ptr == OperandType::OT_UINT64) {
                oss << " " << "uint64";
            } else if (*ptr == OperandType::OT_FLOAT) {
                oss << " " << "float";
            } else if (*ptr == OperandType::OT_FLOAT32) {
                oss << " " << "float32";
            } else if (*ptr == OperandType::OT_FLOAT64) {
                oss << " " << "float64";
            } else {
                oss << " " << "void";
            }
            ++ptr;
        case STREQ:
        case AND:
        case OR:
        case CAPTURE:
        case CAPTURECOPY:
        case CAPTUREMOVE:
        case VEC:
        case VINSERT:
        case VPOP:
        case VAT:
        case INSERT:
        case REMOVE:
            ptr = disassemble_target_register(oss, ptr);

            oss << " " << intop(ptr);
            pointer::inc<bool, byte>(ptr);
            pointer::inc<viua::internals::types::register_index, byte>(ptr);

            oss << " " << intop(ptr);
            pointer::inc<bool, byte>(ptr);
            pointer::inc<viua::internals::types::register_index, byte>(ptr);

            break;
        case JUMP:
            oss << " 0x";
            oss << hex;
            oss << *reinterpret_cast<uint64_t*>(ptr);
            pointer::inc<uint64_t, byte>(ptr);

            oss << dec;

            break;
        case IF:
            oss << " " << intop(ptr);
            pointer::inc<bool, byte>(ptr);
            pointer::inc<viua::internals::types::register_index, byte>(ptr);

            oss << " 0x";
            oss << hex;
            oss << *reinterpret_cast<uint64_t*>(ptr);
            pointer::inc<uint64_t, byte>(ptr);

            oss << " 0x";
            oss << hex;
            oss << *reinterpret_cast<uint64_t*>(ptr);
            pointer::inc<uint64_t, byte>(ptr);

            oss << dec;

            break;
        case FSTORE:
            ptr = disassemble_target_register(oss, ptr);

            oss << " ";
            oss << *reinterpret_cast<float*>(ptr);
            pointer::inc<float, byte>(ptr);
            break;
        case RESS:
            oss << " ";
            switch(int(*ptr)) {
                case 0:
                    oss << "global";
                    break;
                case 1:
                    oss << "local";
                    break;
                case 2:
                    oss << "static";
                    break;
                case 3:
                    oss << "temp";
                    break;
                default:
                    // FIXME: should this only be a warning?
                    oss << "; WARNING: invalid register set type\n";
                    oss << int(*ptr);
            }
            pointer::inc<int, byte>(ptr);
            break;
        case JOIN:
            ptr = disassemble_target_register(oss, ptr);
        case RECEIVE:
            ptr = disassemble_target_register(oss, ptr);

            pointer::inc<bool, byte>(ptr);
            oss << ' ';
            if (decode_timeout(ptr)) {
                oss << decode_timeout(ptr)-1 << "ms";
            } else {
                oss << "infinity";
            }
            pointer::inc<viua::internals::types::timeout, byte>(ptr);

            break;
        default:
            // if opcode was not covered here, it means it must have been a variable-length opcode
            oss << "";
    }

    long increase = (ptr-saved_ptr);
    if (increase < 0) {
        throw ("bytecode pointer increase less than zero: near " + OP_NAMES.at(op) + " instruction");
    }

    // cast increase to unsigned as at this point it is safe to assume that it is greater than zero
    return tuple<string, unsigned>(oss.str(), static_cast<unsigned>(increase));
}
