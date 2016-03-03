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
        byte* nop(byte*);

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

        byte* vec(byte*, int_op);
        byte* vinsert(byte*, int_op, int_op, int_op);
        byte* vpush(byte*, int_op, int_op);
        byte* vpop(byte*, int_op, int_op, int_op);
        byte* vat(byte*, int_op, int_op, int_op);
        byte* vlen(byte*, int_op, int_op);

        byte* lognot(byte*, int_op);
        byte* logand(byte*, int_op, int_op, int_op);
        byte* logor(byte*, int_op, int_op, int_op);

        byte* move(byte*, int_op, int_op);
        byte* copy(byte*, int_op, int_op);
        byte* ref(byte*, int_op, int_op);
        byte* opptr(byte*, int_op, int_op);
        byte* swap(byte*, int_op, int_op);
        byte* opdelete(byte*, int_op);
        byte* empty(byte*, int_op);
        byte* isnull(byte*, int_op, int_op);
        byte* ress(byte*, const std::string&);
        byte* tmpri(byte*, int_op);
        byte* tmpro(byte*, int_op);

        byte* print(byte*, int_op);
        byte* echo(byte*, int_op);

        byte* enclose(byte*, int_op, int_op, int_op);
        byte* openclosecopy(byte*, int_op, int_op, int_op);
        byte* openclosemove(byte*, int_op, int_op, int_op);
        byte* closure(byte*, int_op, const std::string&);
        byte* function(byte*, int_op, const std::string&);
        byte* fcall(byte*, int_op, int_op);

        byte* frame(byte*, int_op, int_op);
        byte* param(byte*, int_op, int_op);
        byte* oppamv(byte*, int_op, int_op);
        byte* paref(byte*, int_op, int_op);
        byte* arg(byte*, int_op, int_op);
        byte* argc(byte*, int_op);
        byte* call(byte*, int_op, const std::string&);
        byte* opthread(byte*, int_op, const std::string&);
        byte* opthjoin(byte*, int_op, int_op);
        byte* opthreceive(byte*, int_op);
        byte* opwatchdog(byte*, const std::string&);

        byte* jump(byte*, uint64_t);
        byte* branch(byte*, int_op, uint64_t, uint64_t);

        byte* vmtry(byte*);
        byte* vmcatch(byte*, const std::string&, const std::string&);
        byte* pull(byte*, int_op);
        byte* vmenter(byte*, const std::string&);
        byte* vmthrow(byte*, int_op);
        byte* leave(byte*);

        byte* opimport(byte*, const std::string&);
        byte* oplink(byte*, const std::string&);

        byte* vmclass(byte*, int_op, const std::string&);
        byte* vmderive(byte*, int_op, const std::string&);
        byte* vmattach(byte*, int_op, const std::string&, const std::string&);
        byte* vmregister(byte*, int_op);
        byte* vmnew(byte*, int_op, const std::string&);
        byte* vmmsg(byte*, int_op, const std::string&);

        byte* opreturn(byte*);
        byte* halt(byte*);
    }
}

#endif
