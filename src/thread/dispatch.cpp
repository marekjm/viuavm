#include <sstream>
#include <viua/bytecode/maps.h>
#include <viua/types/exception.h>
#include <viua/thread.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::dispatch(byte* addr) {
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
        case PTR:
            addr = opptr(addr+1);
            break;
        case SWAP:
            addr = swap(addr+1);
            break;
        case DELETE:
            addr = opdelete(addr+1);
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
        case ENCLOSE:
            addr = enclose(addr+1);
            break;
        case ENCLOSECOPY:
            addr = openclosecopy(addr+1);
            break;
        case ENCLOSEMOVE:
            addr = openclosemove(addr+1);
            break;
        case CLOSURE:
            addr = closure(addr+1);
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
        case PAMV:
            addr = oppamv(addr+1);
            break;
        case PAREF:
            addr = paref(addr+1);
            break;
        case ARG:
            addr = arg(addr+1);
            break;
        case ARGC:
            addr = argc(addr+1);
            break;
        case CALL:
            addr = call(addr+1);
            break;
        case THREAD:
            addr = opthread(addr+1);
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
            addr = jump(addr+1);
            break;
        case BRANCH:
            addr = branch(addr+1);
            break;
        case TRY:
            addr = vmtry(addr+1);
            break;
        case CATCH:
            addr = vmcatch(addr+1);
            break;
        case PULL:
            addr = pull(addr+1);
            break;
        case ENTER:
            addr = vmenter(addr+1);
            break;
        case THROW:
            addr = vmthrow(addr+1);
            break;
        case LEAVE:
            addr = leave(addr+1);
            break;
        case IMPORT:
            addr = opimport(addr+1);
            break;
        case LINK:
            addr = oplink(addr+1);
            break;
        case CLASS:
            addr = vmclass(addr+1);
            break;
        case DERIVE:
            addr = vmderive(addr+1);
            break;
        case ATTACH:
            addr = vmattach(addr+1);
            break;
        case REGISTER:
            addr = vmregister(addr+1);
            break;
        case NEW:
            addr = vmnew(addr+1);
            break;
        case MSG:
            addr = vmmsg(addr+1);
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
