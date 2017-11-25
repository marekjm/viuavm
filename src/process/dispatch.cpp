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

#include <sstream>
#include <viua/bytecode/decoder/operands.h>
#include <viua/bytecode/maps.h>
#include <viua/kernel/kernel.h>
#include <viua/process.h>
#include <viua/types/exception.h>
using namespace std;


auto viua::process::Process::get_trace_line(viua::internals::types::byte* for_address) const -> string {
    ostringstream trace_line;

    trace_line << "[";
    trace_line << " scheduler = " << hex << scheduler << dec;
    trace_line << ", process = " << hex << this << dec;
    trace_line << ", stack = " << hex << stack << dec;
    trace_line << ", frame = 0x" << hex << reinterpret_cast<unsigned long>(stack->back().get()) << dec;
    trace_line << ", jump_base = 0x" << hex << reinterpret_cast<unsigned long>(stack->jump_base) << dec;
    trace_line << ", address = 0x" << hex << reinterpret_cast<unsigned long>(for_address) << dec;
    trace_line << ", depth = " << stack->size();
    if (stack->thrown) {
        trace_line << ", thrown = 0x" << hex << reinterpret_cast<unsigned long>(stack->thrown.get()) << dec;
    }
    if (stack->caught) {
        trace_line << ", caught = 0x" << hex << reinterpret_cast<unsigned long>(stack->caught.get()) << dec;
    }
    trace_line << " ] ";

    try {
        trace_line << OP_NAMES.at(static_cast<OPCODE>(*for_address));
    } catch (std::out_of_range& e) {
        trace_line << "<unrecognised instruction byte = " << static_cast<unsigned>(*for_address) << '>';
        return trace_line.str();
    }

    if (static_cast<OPCODE>(*for_address) == CALL or static_cast<OPCODE>(*for_address) == PROCESS or
        static_cast<OPCODE>(*for_address) == MSG) {
        auto working_address = for_address + 1;
        if (viua::bytecode::decoder::operands::is_void(working_address)) {
            ++working_address;
        } else {
            working_address += sizeof(viua::internals::types::byte);  // for opcode type
            working_address += sizeof(viua::internals::types::register_index);
            working_address += sizeof(viua::internals::types::registerset_type_marker);
        }
        trace_line << ' ' << string(reinterpret_cast<char*>(working_address));
    }
    if (static_cast<OPCODE>(*for_address) == TAILCALL or static_cast<OPCODE>(*for_address) == DEFER) {
        trace_line << ' ';
        trace_line << string(reinterpret_cast<char*>(for_address + 1));
    }
    if (static_cast<OPCODE>(*for_address) == RETURN) {
        trace_line << " from " + stack->back()->function_name;
        if (stack->state_of() == viua::process::Stack::STATE::RUNNING) {
            trace_line << (stack->back()->deferred_calls.size() ? " before deferred" : " with no deferred");
        } else if (stack->state_of() == viua::process::Stack::STATE::SUSPENDED_BY_DEFERRED_ON_FRAME_POP) {
            trace_line << " after deferred";
        }
    }

    return trace_line.str();
}
auto viua::process::Process::emit_trace_line(viua::internals::types::byte* for_address) const -> void {
    // FIXME conditionally enable duplicate trace lines
    static string previous_trace_line;
    string line = get_trace_line(for_address);

    if (line != previous_trace_line) {
        cerr << (line + "\n");
        previous_trace_line = line;
    }
}


viua::internals::types::byte* viua::process::Process::dispatch(viua::internals::types::byte* addr) {
    /** Dispatches instruction at a pointer to its handler.
     */
    if (tracing_enabled) {
        emit_trace_line(addr);
    }
    switch (static_cast<OPCODE>(*addr)) {
        case IZERO:
            addr = opizero(addr + 1);
            break;
        case INTEGER:
            addr = opinteger(addr + 1);
            break;
        case IINC:
            addr = opiinc(addr + 1);
            break;
        case IDEC:
            addr = opidec(addr + 1);
            break;
        case FLOAT:
            addr = opfloat(addr + 1);
            break;
        case ITOF:
            addr = opitof(addr + 1);
            break;
        case FTOI:
            addr = opftoi(addr + 1);
            break;
        case STOI:
            addr = opstoi(addr + 1);
            break;
        case STOF:
            addr = opstof(addr + 1);
            break;
        case ADD:
            addr = opadd(addr + 1);
            break;
        case SUB:
            addr = opsub(addr + 1);
            break;
        case MUL:
            addr = opmul(addr + 1);
            break;
        case DIV:
            addr = opdiv(addr + 1);
            break;
        case LT:
            addr = oplt(addr + 1);
            break;
        case LTE:
            addr = oplte(addr + 1);
            break;
        case GT:
            addr = opgt(addr + 1);
            break;
        case GTE:
            addr = opgte(addr + 1);
            break;
        case EQ:
            addr = opeq(addr + 1);
            break;
        case STRING:
            addr = opstring(addr + 1);
            break;
        case TEXT:
            addr = optext(addr + 1);
            break;
        case TEXTEQ:
            addr = optexteq(addr + 1);
            break;
        case TEXTAT:
            addr = optextat(addr + 1);
            break;
        case TEXTSUB:
            addr = optextsub(addr + 1);
            break;
        case TEXTLENGTH:
            addr = optextlength(addr + 1);
            break;
        case TEXTCOMMONPREFIX:
            addr = optextcommonprefix(addr + 1);
            break;
        case TEXTCOMMONSUFFIX:
            addr = optextcommonsuffix(addr + 1);
            break;
        case TEXTCONCAT:
            addr = optextconcat(addr + 1);
            break;
        case VECTOR:
            addr = opvector(addr + 1);
            break;
        case VINSERT:
            addr = opvinsert(addr + 1);
            break;
        case VPUSH:
            addr = opvpush(addr + 1);
            break;
        case VPOP:
            addr = opvpop(addr + 1);
            break;
        case VAT:
            addr = opvat(addr + 1);
            break;
        case VLEN:
            addr = opvlen(addr + 1);
            break;
        case NOT:
            addr = opnot(addr + 1);
            break;
        case AND:
            addr = opand(addr + 1);
            break;
        case OR:
            addr = opor(addr + 1);
            break;
        case BITS:
            addr = opbits(addr + 1);
            break;
        case BITAND:
            addr = opbitand(addr + 1);
            break;
        case BITOR:
            addr = opbitor(addr + 1);
            break;
        case BITNOT:
            addr = opbitnot(addr + 1);
            break;
        case BITXOR:
            addr = opbitxor(addr + 1);
            break;
        case BITAT:
            addr = opbitat(addr + 1);
            break;
        case BITSET:
            addr = opbitset(addr + 1);
            break;
        case SHL:
            addr = opshl(addr + 1);
            break;
        case SHR:
            addr = opshr(addr + 1);
            break;
        case ASHL:
            addr = opashl(addr + 1);
            break;
        case ASHR:
            addr = opashr(addr + 1);
            break;
        case ROL:
            addr = oprol(addr + 1);
            break;
        case ROR:
            addr = opror(addr + 1);
            break;
        case WRAPINCREMENT:
            addr = opwrapincrement(addr + 1);
            break;
        case WRAPDECREMENT:
            addr = opwrapdecrement(addr + 1);
            break;
        case WRAPADD:
            addr = opwrapadd(addr + 1);
            break;
        case WRAPMUL:
            addr = opwrapmul(addr + 1);
            break;
        case WRAPDIV:
            addr = opwrapdiv(addr + 1);
            break;
        case CHECKEDSINCREMENT:
            addr = opcheckedsincrement(addr + 1);
            break;
        case CHECKEDSDECREMENT:
            addr = opcheckedsdecrement(addr + 1);
            break;
        case CHECKEDSADD:
            addr = opcheckedsadd(addr + 1);
            break;
        case CHECKEDSMUL:
            addr = opcheckedsmul(addr + 1);
            break;
        case CHECKEDSDIV:
            addr = opcheckedsdiv(addr + 1);
            break;
        case SATURATINGSINCREMENT:
            addr = opsaturatingsincrement(addr + 1);
            break;
        case SATURATINGSDECREMENT:
            addr = opsaturatingsdecrement(addr + 1);
            break;
        case SATURATINGSADD:
            addr = opsaturatingsadd(addr + 1);
            break;
        case SATURATINGSMUL:
            addr = opsaturatingsmul(addr + 1);
            break;
        case SATURATINGSDIV:
            addr = opsaturatingsdiv(addr + 1);
            break;
        case MOVE:
            addr = opmove(addr + 1);
            break;
        case COPY:
            addr = opcopy(addr + 1);
            break;
        case PTR:
            addr = opptr(addr + 1);
            break;
        case SWAP:
            addr = opswap(addr + 1);
            break;
        case DELETE:
            addr = opdelete(addr + 1);
            break;
        case ISNULL:
            addr = opisnull(addr + 1);
            break;
        case RESS:
            addr = opress(addr + 1);
            break;
        case PRINT:
            addr = opprint(addr + 1);
            break;
        case ECHO:
            addr = opecho(addr + 1);
            break;
        case CAPTURE:
            addr = opcapture(addr + 1);
            break;
        case CAPTURECOPY:
            addr = opcapturecopy(addr + 1);
            break;
        case CAPTUREMOVE:
            addr = opcapturemove(addr + 1);
            break;
        case CLOSURE:
            addr = opclosure(addr + 1);
            break;
        case FUNCTION:
            addr = opfunction(addr + 1);
            break;
        case FRAME:
            addr = opframe(addr + 1);
            break;
        case PARAM:
            addr = opparam(addr + 1);
            break;
        case PAMV:
            addr = oppamv(addr + 1);
            break;
        case ARG:
            addr = oparg(addr + 1);
            break;
        case ARGC:
            addr = opargc(addr + 1);
            break;
        case CALL:
            addr = opcall(addr + 1);
            break;
        case TAILCALL:
            addr = optailcall(addr + 1);
            break;
        case DEFER:
            addr = opdefer(addr + 1);
            break;
        case PROCESS:
            addr = opprocess(addr + 1);
            break;
        case SELF:
            addr = opself(addr + 1);
            break;
        case JOIN:
            addr = opjoin(addr + 1);
            break;
        case SEND:
            addr = opsend(addr + 1);
            break;
        case RECEIVE:
            addr = opreceive(addr + 1);
            break;
        case WATCHDOG:
            addr = opwatchdog(addr + 1);
            break;
        case RETURN:
            addr = opreturn(addr);
            break;
        case JUMP:
            addr = opjump(addr + 1);
            break;
        case IF:
            addr = opif(addr + 1);
            break;
        case TRY:
            addr = optry(addr + 1);
            break;
        case CATCH:
            addr = opcatch(addr + 1);
            break;
        case DRAW:
            addr = opdraw(addr + 1);
            break;
        case ENTER:
            addr = openter(addr + 1);
            break;
        case THROW:
            addr = opthrow(addr + 1);
            break;
        case LEAVE:
            addr = opleave(addr + 1);
            break;
        case IMPORT:
            addr = opimport(addr + 1);
            break;
        case CLASS:
            addr = opclass(addr + 1);
            break;
        case DERIVE:
            addr = opderive(addr + 1);
            break;
        case ATTACH:
            addr = opattach(addr + 1);
            break;
        case REGISTER:
            addr = opregister(addr + 1);
            break;
        case ATOM:
            addr = opatom(addr + 1);
            break;
        case ATOMEQ:
            addr = opatomeq(addr + 1);
            break;
        case STRUCT:
            addr = opstruct(addr + 1);
            break;
        case STRUCTINSERT:
            addr = opstructinsert(addr + 1);
            break;
        case STRUCTREMOVE:
            addr = opstructremove(addr + 1);
            break;
        case STRUCTKEYS:
            addr = opstructkeys(addr + 1);
            break;
        case NEW:
            addr = opnew(addr + 1);
            break;
        case MSG:
            addr = opmsg(addr + 1);
            break;
        case INSERT:
            addr = opinsert(addr + 1);
            break;
        case REMOVE:
            addr = opremove(addr + 1);
            break;
        case HALT:
            stack->state_of(Stack::STATE::HALTED);
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
            throw new viua::types::Exception(error.str());
    }
    return addr;
}
