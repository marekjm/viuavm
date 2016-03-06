#include <sstream>
#include <viua/bytecode/maps.h>
#include <viua/types/exception.h>
#include <viua/process.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Process::dispatch(byte* addr) {
    /** Dispatches instruction at a pointer to its handler.
     */
    switch (static_cast<OPCODE>(*addr)) {
        case IZERO:
            addr = opizero(addr+1);
            break;
        case ISTORE:
            addr = opistore(addr+1);
            break;
        case IADD:
            addr = opiadd(addr+1);
            break;
        case ISUB:
            addr = opisub(addr+1);
            break;
        case IMUL:
            addr = opimul(addr+1);
            break;
        case IDIV:
            addr = opidiv(addr+1);
            break;
        case IINC:
            addr = opiinc(addr+1);
            break;
        case IDEC:
            addr = opidec(addr+1);
            break;
        case ILT:
            addr = opilt(addr+1);
            break;
        case ILTE:
            addr = opilte(addr+1);
            break;
        case IGT:
            addr = opigt(addr+1);
            break;
        case IGTE:
            addr = opigte(addr+1);
            break;
        case IEQ:
            addr = opieq(addr+1);
            break;
        case FSTORE:
            addr = opfstore(addr+1);
            break;
        case FADD:
            addr = opfadd(addr+1);
            break;
        case FSUB:
            addr = opfsub(addr+1);
            break;
        case FMUL:
            addr = opfmul(addr+1);
            break;
        case FDIV:
            addr = opfdiv(addr+1);
            break;
        case FLT:
            addr = opflt(addr+1);
            break;
        case FLTE:
            addr = opflte(addr+1);
            break;
        case FGT:
            addr = opfgt(addr+1);
            break;
        case FGTE:
            addr = opfgte(addr+1);
            break;
        case FEQ:
            addr = opfeq(addr+1);
            break;
        case BSTORE:
            addr = opbstore(addr+1);
            break;
        case ITOF:
            addr = opitof(addr+1);
            break;
        case FTOI:
            addr = opftoi(addr+1);
            break;
        case STOI:
            addr = opstoi(addr+1);
            break;
        case STOF:
            addr = opstof(addr+1);
            break;
        case STRSTORE:
            addr = opstrstore(addr+1);
            break;
        case VEC:
            addr = opvec(addr+1);
            break;
        case VINSERT:
            addr = opvinsert(addr+1);
            break;
        case VPUSH:
            addr = opvpush(addr+1);
            break;
        case VPOP:
            addr = opvpop(addr+1);
            break;
        case VAT:
            addr = opvat(addr+1);
            break;
        case VLEN:
            addr = opvlen(addr+1);
            break;
        case NOT:
            addr = opnot(addr+1);
            break;
        case AND:
            addr = opand(addr+1);
            break;
        case OR:
            addr = opor(addr+1);
            break;
        case MOVE:
            addr = opmove(addr+1);
            break;
        case COPY:
            addr = opcopy(addr+1);
            break;
        case REF:
            addr = opref(addr+1);
            break;
        case PTR:
            addr = opptr(addr+1);
            break;
        case SWAP:
            addr = opswap(addr+1);
            break;
        case DELETE:
            addr = opdelete(addr+1);
            break;
        case EMPTY:
            addr = opempty(addr+1);
            break;
        case ISNULL:
            addr = opisnull(addr+1);
            break;
        case RESS:
            addr = opress(addr+1);
            break;
        case TMPRI:
            addr = optmpri(addr+1);
            break;
        case TMPRO:
            addr = optmpro(addr+1);
            break;
        case PRINT:
              addr = opprint(addr+1);
            break;
        case ECHO:
              addr = opecho(addr+1);
            break;
        case ENCLOSE:
              addr = openclose(addr+1);
            break;
        case ENCLOSECOPY:
            addr = openclosecopy(addr+1);
            break;
        case ENCLOSEMOVE:
            addr = openclosemove(addr+1);
            break;
        case CLOSURE:
              addr = opclosure(addr+1);
            break;
        case FUNCTION:
              addr = opfunction(addr+1);
            break;
        case FCALL:
              addr = opfcall(addr+1);
            break;
        case FRAME:
              addr = opframe(addr+1);
            break;
        case PARAM:
              addr = opparam(addr+1);
            break;
        case PAMV:
            addr = oppamv(addr+1);
            break;
        case PAREF:
              addr = opparef(addr+1);
            break;
        case ARG:
              addr = oparg(addr+1);
            break;
        case ARGC:
              addr = opargc(addr+1);
            break;
        case CALL:
              addr = opcall(addr+1);
            break;
        case PROCESS:
            addr = opprocess(addr+1);
            break;
        case THJOIN:
            addr = opthjoin(addr+1);
            break;
        case THRECEIVE:
            addr = opthreceive(addr+1);
            break;
        case WATCHDOG:
            addr = opwatchdog(addr+1);
            break;
        case RETURN:
            addr = opreturn(addr);
            break;
        case JUMP:
              addr = opjump(addr+1);
            break;
        case BRANCH:
              addr = opbranch(addr+1);
            break;
        case TRY:
            addr = optry(addr+1);
            break;
        case CATCH:
            addr = opcatch(addr+1);
            break;
        case PULL:
              addr = oppull(addr+1);
            break;
        case ENTER:
            addr = openter(addr+1);
            break;
        case THROW:
            addr = opthrow(addr+1);
            break;
        case LEAVE:
              addr = opleave(addr+1);
            break;
        case IMPORT:
            addr = opimport(addr+1);
            break;
        case LINK:
            addr = oplink(addr+1);
            break;
        case CLASS:
            addr = opclass(addr+1);
            break;
        case DERIVE:
            addr = opderive(addr+1);
            break;
        case ATTACH:
            addr = opattach(addr+1);
            break;
        case REGISTER:
            addr = opregister(addr+1);
            break;
        case NEW:
            addr = opnew(addr+1);
            break;
        case MSG:
            addr = opmsg(addr+1);
            break;
        case HALT:
            throw HaltException();
            break;
        case NOP:
            ++addr;
            break;
        default:
            ostringstream error;
            error << "unrecognised instruction (byte value " << int(*addr) << ")";
            if (OP_NAMES.count(static_cast<OPCODE>(*addr))) {
                error << ": " << OP_NAMES.at(static_cast<OPCODE>(*addr));
            }
            throw new Exception(error.str());
    }
    return addr;
}
