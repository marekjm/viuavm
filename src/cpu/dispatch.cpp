#include <sstream>
#include "../bytecode/bytetypedef.h"
#include "../bytecode/opcodes.h"
#include "cpu.h"
using namespace std;


byte* CPU::dispatch(byte* addr) {
    /** Dispatches instruction at a pointer to its handler.
     */
    switch (*addr) {
        case IZERO:
            addr = izero(addr+1);
            break;
        case ISTORE:
            addr = istore(addr+1);
            break;
        case IADD:
            addr = iadd(addr+1);
            break;
        case ISUB:
            addr = isub(addr+1);
            break;
        case IMUL:
            addr = imul(addr+1);
            break;
        case IDIV:
            addr = idiv(addr+1);
            break;
        case IINC:
            addr = iinc(addr+1);
            break;
        case IDEC:
            addr = idec(addr+1);
            break;
        case ILT:
            addr = ilt(addr+1);
            break;
        case ILTE:
            addr = ilte(addr+1);
            break;
        case IGT:
            addr = igt(addr+1);
            break;
        case IGTE:
            addr = igte(addr+1);
            break;
        case IEQ:
            addr = ieq(addr+1);
            break;
        case FSTORE:
            addr = fstore(addr+1);
            break;
        case FADD:
            addr = fadd(addr+1);
            break;
        case FSUB:
            addr = fsub(addr+1);
            break;
        case FMUL:
            addr = fmul(addr+1);
            break;
        case FDIV:
            addr = fdiv(addr+1);
            break;
        case FLT:
            addr = flt(addr+1);
            break;
        case FLTE:
            addr = flte(addr+1);
            break;
        case FGT:
            addr = fgt(addr+1);
            break;
        case FGTE:
            addr = fgte(addr+1);
            break;
        case FEQ:
            addr = feq(addr+1);
            break;
        case BSTORE:
            addr = bstore(addr+1);
            break;
        case ITOF:
            addr = itof(addr+1);
            break;
        case FTOI:
            addr = ftoi(addr+1);
            break;
        case STOI:
            addr = stoi(addr+1);
            break;
        case STOF:
            addr = stof(addr+1);
            break;
        case STRSTORE:
            addr = strstore(addr+1);
            break;
        case VEC:
            addr = vec(addr+1);
            break;
        case VINSERT:
            addr = vinsert(addr+1);
            break;
        case VPUSH:
            addr = vpush(addr+1);
            break;
        case VPOP:
            addr = vpop(addr+1);
            break;
        case VAT:
            addr = vat(addr+1);
            break;
        case VLEN:
            addr = vlen(addr+1);
            break;
        case NOT:
            addr = lognot(addr+1);
            break;
        case AND:
            addr = logand(addr+1);
            break;
        case OR:
            addr = logor(addr+1);
            break;
        case MOVE:
            addr = move(addr+1);
            break;
        case COPY:
            addr = copy(addr+1);
            break;
        case REF:
            addr = ref(addr+1);
            break;
        case SWAP:
            addr = swap(addr+1);
            break;
        case FREE:
            addr = free(addr+1);
            break;
        case EMPTY:
            addr = empty(addr+1);
            break;
        case ISNULL:
            addr = isnull(addr+1);
            break;
        case RESS:
            addr = ress(addr+1);
            break;
        case TMPRI:
            addr = tmpri(addr+1);
            break;
        case TMPRO:
            addr = tmpro(addr+1);
            break;
        case PRINT:
            addr = print(addr+1);
            break;
        case ECHO:
            addr = echo(addr+1);
            break;
        case CLBIND:
            addr = clbind(addr+1);
            break;
        case CLOSURE:
            addr = closure(addr+1);
            break;
        case CLFRAME:
            addr = clframe(addr+1);
            break;
        case CLCALL:
            addr = clcall(addr+1);
            break;
        case FUNCTION:
            addr = function(addr+1);
            break;
        case FCALL:
            addr = fcall(addr+1);
            break;
        case FRAME:
            addr = frame(addr+1);
            break;
        case PARAM:
            addr = param(addr+1);
            break;
        case PAREF:
            addr = paref(addr+1);
            break;
        case ARG:
            addr = arg(addr+1);
            break;
        case CALL:
            addr = call(addr+1);
            break;
        case END:
            addr = end(addr);
            break;
        case JUMP:
            addr = jump(addr+1);
            break;
        case BRANCH:
            addr = branch(addr+1);
            break;
        case TRYFRAME:
            addr = tryframe(addr+1);
            break;
        case CATCH:
            addr = vmcatch(addr+1);
            break;
        case PULL:
            addr = pull(addr+1);
            break;
        case TRY:
            addr = vmtry(addr+1);
            break;
        case THROW:
            addr = vmthrow(addr+1);
            break;
        case LEAVE:
            addr = leave(addr+1);
            break;
        case EXIMPORT:
            addr = eximport(addr+1);
            break;
        case EXCALL:
            addr = excall(addr+1);
            break;
        case HALT:
            throw HaltException();
            break;
        case NOP:
            ++addr;
            break;
        default:
            ostringstream error;
            error << "unrecognised instruction (bytecode value: " << int(*((byte*)bytecode)) << ")";
            throw error.str().c_str();
    }
    return addr;
}
