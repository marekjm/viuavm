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

        byte* izero(byte*, int_op);
        byte* istore(byte*, int_op, int_op);
        byte* iadd(byte*, int_op, int_op, int_op);
        byte* isub(byte*, int_op, int_op, int_op);
        byte* imul(byte*, int_op, int_op, int_op);
        byte* idiv(byte*, int_op, int_op, int_op);
        byte* iinc(byte*, int_op);
        byte* idec(byte*, int_op);
        byte* ilt(byte*, int_op, int_op, int_op);
        byte* ilte(byte*, int_op, int_op, int_op);
        byte* igt(byte*, int_op, int_op, int_op);
        byte* igte(byte*, int_op, int_op, int_op);
        byte* ieq(byte*, int_op, int_op, int_op);

        byte* fstore(byte*, int_op, float);
        byte* fadd(byte*, int_op, int_op, int_op);
        byte* fsub(byte*, int_op, int_op, int_op);
        byte* fmul(byte*, int_op, int_op, int_op);
        byte* fdiv(byte*, int_op, int_op, int_op);
        byte* flt(byte*, int_op, int_op, int_op);
        byte* flte(byte*, int_op, int_op, int_op);
        byte* fgt(byte*, int_op, int_op, int_op);
        byte* fgte(byte*, int_op, int_op, int_op);
        byte* feq(byte*, int_op, int_op, int_op);

        byte* bstore(byte*, int_op, byte_op);

        byte* itof(byte*, int_op, int_op);
        byte* ftoi(byte*, int_op, int_op);
        byte* stoi(byte*, int_op, int_op);
        byte* stof(byte*, int_op, int_op);

        byte* strstore(byte*, int_op, std::string);

        byte* vec(byte*, int_op);
        byte* vinsert(byte*, int_op, int_op, int_op);
        byte* vpush(byte*, int_op, int_op);
        byte* vpop(byte*, int_op, int_op, int_op);
        byte* vat(byte*, int_op, int_op, int_op);
        byte* vlen(byte*, int_op, int_op);

        byte* lognot(byte* addr_ptr, int_op reg);
        byte* logand(byte* addr_ptr, int_op rega, int_op regb, int_op regr);
        byte* logor(byte* addr_ptr, int_op rega, int_op regb, int_op regr);

        byte* move(byte*, int_op, int_op);
        byte* copy(byte*, int_op, int_op);
        byte* ref(byte*, int_op, int_op);
        byte* swap(byte*, int_op, int_op);
        byte* free(byte*, int_op);
        byte* empty(byte*, int_op);
        byte* isnull(byte*, int_op, int_op);
        byte* ress(byte*, const std::string&);
        byte* tmpri(byte*, int_op);
        byte* tmpro(byte*, int_op);

        byte* print(byte*, int_op);
        byte* echo(byte*, int_op);

        byte* clbind(byte*, int_op);
        byte* closure(byte*, const std::string&, int_op);
        byte* function(byte*, const std::string&, int_op);
        byte* fcall(byte*, int_op, int_op);

        byte* frame(byte*, int_op, int_op);
        byte* param(byte*, int_op, int_op);
        byte* paref(byte*, int_op, int_op);
        byte* arg(byte*, int_op, int_op);
        byte* call(byte*, const std::string&, int_op);

        byte* jump(byte*, int);
        byte* branch(byte*, int_op, int, int);

        byte* tryframe(byte*);
        byte* vmcatch(byte*, const std::string&, const std::string&);
        byte* pull(byte*, int_op);
        byte* vmtry(byte*, const std::string&);
        byte* vmthrow(byte*, int_op);
        byte* leave(byte*);

        byte* eximport(byte*, const std::string&);
        byte* excall(byte*, const std::string&, int_op);

        byte* end(byte*);
        byte* halt(byte*);
    }
}

#endif
