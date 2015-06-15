#include <sstream>
#include "../../bytecode/opcodes.h"
#include "../../bytecode/maps.h"
#include "../../support/pointer.h"
#include "disassembler.h"
using namespace std;


string disassembler::intop(byte* ptr) {
    ostringstream oss;

    oss << ((*(bool*)ptr) ? "@" : "");
    pointer::inc<bool, byte>(ptr);
    oss << *(int*)ptr;

    return oss.str();
}

tuple<string, unsigned> disassembler::instruction(byte* ptr) {
    byte* bptr = ptr;

    OPCODE op = OPCODE(*bptr);
    string opname = OP_NAMES.at(op);

    ++bptr; // instruction byte is not needed anymore

    ostringstream oss;
    oss << opname;
    if (in(OP_VARIABLE_LENGTH, op)) {
        bptr += string(bptr).size();
        ++bptr; // for null character terminating the C-style string not included in std::string
        bptr += (OP_SIZES.at(opname)-1); // -1 because OP_SIZES add one for instruction-storing byte
    } else {
        bptr += (OP_SIZES.at(opname)-1); // -1 because OP_SIZES add one for instruction-storing byte
    }

    unsigned increase = (bptr-ptr);

    ++ptr;
    switch (op) {
        case IZERO:
        case IINC:
        case IDEC:
        case BINC:
        case BDEC:
        case PRINT:
        case ECHO:
        case BOOL:
        case NOT:
        case FREE:
        case EMPTY:
        case TMPRI:
        case TMPRO:
            oss << " " << intop(ptr);
            pointer::inc<bool, byte>(ptr);
            pointer::inc<int, byte>(ptr);

            break;
        case ISTORE:
        case ITOF:
        case FTOI:
        case STOI:
        case STOF:
        case FRAME:
        case ARG:
        case PARAM:
        case PAREF:
        case MOVE:
        case COPY:
        case REF:
        case SWAP:
        case ISNULL:
            oss << " " << intop(ptr);
            pointer::inc<bool, byte>(ptr);
            pointer::inc<int, byte>(ptr);

            oss << " " << intop(ptr);
            pointer::inc<bool, byte>(ptr);
            pointer::inc<int, byte>(ptr);

            break;
        case IADD:
        case ISUB:
        case IMUL:
        case IDIV:
        case ILT:
        case ILTE:
        case IGT:
        case IGTE:
        case IEQ:
        case FADD:
        case FSUB:
        case FMUL:
        case FDIV:
        case FLT:
        case FLTE:
        case FGT:
        case FGTE:
        case FEQ:
        case BADD:
        case BSUB:
        case BLT:
        case BLTE:
        case BGT:
        case BGTE:
        case BEQ:
        case AND:
        case OR:
            oss << " " << intop(ptr);
            pointer::inc<bool, byte>(ptr);
            pointer::inc<int, byte>(ptr);

            oss << " " << intop(ptr);
            pointer::inc<bool, byte>(ptr);
            pointer::inc<int, byte>(ptr);

            oss << " " << intop(ptr);
            pointer::inc<bool, byte>(ptr);
            pointer::inc<int, byte>(ptr);

            break;
        case BRANCH:
            oss << " " << intop(ptr);
            pointer::inc<bool, byte>(ptr);
            pointer::inc<int, byte>(ptr);

            oss << " 0x";
            oss << hex;
            oss << *(int*)ptr;
            pointer::inc<int, byte>(ptr);

            oss << " 0x";
            oss << hex;
            oss << *(int*)ptr;
            pointer::inc<int, byte>(ptr);

            oss << dec;

            break;
        case FSTORE:
            oss << " " << intop(ptr);
            pointer::inc<bool, byte>(ptr);
            pointer::inc<int, byte>(ptr);

            oss << " ";
            oss << *(float*)ptr;
            break;
        case BSTORE:
            oss << " " << intop(ptr);
            pointer::inc<bool, byte>(ptr);
            pointer::inc<int, byte>(ptr);

            oss << " ";
            pointer::inc<bool, byte>(ptr);
            oss << int(*(byte*)ptr);
            break;
        case RESS:
            oss << " ";
            oss << int(*(byte*)ptr);
            break;
        default:
            oss << "";
    }
    return tuple<string, unsigned>(oss.str(), increase);
}
