#ifndef VIUA_CG_BYTECODE_INSTRUCTIONS_H
#define VIUA_CG_BYTECODE_INSTRUCTIONS_H

#pragma once

#include <string>
#include <tuple>
#include "../../bytecode/opcodes.h"
#include "../../support/pointer.h"


/** typedefs for various types of operands
 *  that Viua asm instructions may use.
 */
typedef std::tuple<bool, int> int_op;
typedef std::tuple<bool, byte> byte_op;
typedef std::tuple<bool, float> float_op;

namespace cg {
    namespace bytecode {
        byte* opnop(byte*);

        byte* opizero(byte*, int_op);
        byte* opistore(byte*, int_op, int_op);
        byte* opiadd(byte*, int_op, int_op, int_op);
        byte* opisub(byte*, int_op, int_op, int_op);
        byte* opimul(byte*, int_op, int_op, int_op);
        byte* opidiv(byte*, int_op, int_op, int_op);
        byte* opiinc(byte*, int_op);
        byte* opidec(byte*, int_op);
        byte* opilt(byte*, int_op, int_op, int_op);
        byte* opilte(byte*, int_op, int_op, int_op);
        byte* opigt(byte*, int_op, int_op, int_op);
        byte* opigte(byte*, int_op, int_op, int_op);
        byte* opieq(byte*, int_op, int_op, int_op);

        byte* opfstore(byte*, int_op, float);
        byte* opfadd(byte*, int_op, int_op, int_op);
        byte* opfsub(byte*, int_op, int_op, int_op);
        byte* opfmul(byte*, int_op, int_op, int_op);
        byte* opfdiv(byte*, int_op, int_op, int_op);
        byte* opflt(byte*, int_op, int_op, int_op);
        byte* opflte(byte*, int_op, int_op, int_op);
        byte* opfgt(byte*, int_op, int_op, int_op);
        byte* opfgte(byte*, int_op, int_op, int_op);
        byte* opfeq(byte*, int_op, int_op, int_op);

        byte* opbstore(byte*, int_op, byte_op);

        byte* opitof(byte*, int_op, int_op);
        byte* opftoi(byte*, int_op, int_op);
        byte* opstoi(byte*, int_op, int_op);
        byte* opstof(byte*, int_op, int_op);

        byte* opstrstore(byte*, int_op, std::string);

        byte* opvec(byte*, int_op);
        byte* opvinsert(byte*, int_op, int_op, int_op);
        byte* opvpush(byte*, int_op, int_op);
        byte* opvpop(byte*, int_op, int_op, int_op);
        byte* opvat(byte*, int_op, int_op, int_op);
        byte* opvlen(byte*, int_op, int_op);

        byte* opnot(byte*, int_op);
        byte* opand(byte*, int_op, int_op, int_op);
        byte* opor(byte*, int_op, int_op, int_op);

        byte* opmove(byte*, int_op, int_op);
        byte* opcopy(byte*, int_op, int_op);
        byte* opref(byte*, int_op, int_op);
        byte* opptr(byte*, int_op, int_op);
        byte* opswap(byte*, int_op, int_op);
        byte* opdelete(byte*, int_op);
        byte* opempty(byte*, int_op);
        byte* opisnull(byte*, int_op, int_op);
        byte* opress(byte*, const std::string&);
        byte* optmpri(byte*, int_op);
        byte* optmpro(byte*, int_op);

        byte* opprint(byte*, int_op);
        byte* opecho(byte*, int_op);

        byte* openclose(byte*, int_op, int_op, int_op);
        byte* openclosecopy(byte*, int_op, int_op, int_op);
        byte* openclosemove(byte*, int_op, int_op, int_op);
        byte* opclosure(byte*, int_op, const std::string&);
        byte* opfunction(byte*, int_op, const std::string&);
        byte* opfcall(byte*, int_op, int_op);

        byte* opframe(byte*, int_op, int_op);
        byte* opparam(byte*, int_op, int_op);
        byte* oppamv(byte*, int_op, int_op);
        byte* opparef(byte*, int_op, int_op);
        byte* oparg(byte*, int_op, int_op);
        byte* opargc(byte*, int_op);
        byte* opcall(byte*, int_op, const std::string&);
        byte* opprocess(byte*, int_op, const std::string&);
        byte* opthjoin(byte*, int_op, int_op);
        byte* opthreceive(byte*, int_op);
        byte* opwatchdog(byte*, const std::string&);

        byte* opjump(byte*, uint64_t);
        byte* opbranch(byte*, int_op, uint64_t, uint64_t);

        byte* optry(byte*);
        byte* opcatch(byte*, const std::string&, const std::string&);
        byte* oppull(byte*, int_op);
        byte* openter(byte*, const std::string&);
        byte* opthrow(byte*, int_op);
        byte* opleave(byte*);

        byte* opimport(byte*, const std::string&);
        byte* oplink(byte*, const std::string&);

        byte* opclass(byte*, int_op, const std::string&);
        byte* opderive(byte*, int_op, const std::string&);
        byte* opattach(byte*, int_op, const std::string&, const std::string&);
        byte* opregister(byte*, int_op);
        byte* opnew(byte*, int_op, const std::string&);
        byte* opmsg(byte*, int_op, const std::string&);

        byte* opreturn(byte*);
        byte* ophalt(byte*);
    }
}

#endif
