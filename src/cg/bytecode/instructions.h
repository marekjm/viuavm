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

enum JUMPTYPE {
    JMP_RELATIVE = 0,
    JMP_ABSOLUTE,
    JMP_TO_BYTE,
};

namespace cg {
    namespace bytecode {
        byte* nop(byte* addr_ptr);

        byte* izero(byte* addr_ptr, int_op regno);
        byte* istore(byte* addr_ptr, int_op regno, int_op i);
        byte* iadd(byte* addr_ptr, int_op rega, int_op regb, int_op regr);
        byte* isub(byte* addr_ptr, int_op rega, int_op regb, int_op regr);
        byte* imul(byte* addr_ptr, int_op rega, int_op regb, int_op regr);
        byte* idiv(byte* addr_ptr, int_op rega, int_op regb, int_op regr);
        byte* iinc(byte* addr_ptr, int_op regno);
        byte* idec(byte* addr_ptr, int_op regno);
        byte* ilt(byte* addr_ptr, int_op rega, int_op regb, int_op regr);
        byte* ilte(byte* addr_ptr, int_op rega, int_op regb, int_op regr);
        byte* igt(byte* addr_ptr, int_op rega, int_op regb, int_op regr);
        byte* igte(byte* addr_ptr, int_op rega, int_op regb, int_op regr);
        byte* ieq(byte* addr_ptr, int_op rega, int_op regb, int_op regr);

        byte* fstore(byte* addr_ptr, int_op regno, float f);
        byte* fadd(byte* addr_ptr, int_op rega, int_op regb, int_op regr);
        byte* fsub(byte* addr_ptr, int_op rega, int_op regb, int_op regr);
        byte* fmul(byte* addr_ptr, int_op rega, int_op regb, int_op regr);
        byte* fdiv(byte* addr_ptr, int_op rega, int_op regb, int_op regr);
        byte* flt(byte* addr_ptr, int_op rega, int_op regb, int_op regr);
        byte* flte(byte* addr_ptr, int_op rega, int_op regb, int_op regr);
        byte* fgt(byte* addr_ptr, int_op rega, int_op regb, int_op regr);
        byte* fgte(byte* addr_ptr, int_op rega, int_op regb, int_op regr);
        byte* feq(byte* addr_ptr, int_op rega, int_op regb, int_op regr);

        byte* bstore(byte* addr_ptr, int_op regno, byte_op b);

        byte* itof(byte* addr_ptr, int_op a, int_op b);
        byte* ftoi(byte* addr_ptr, int_op a, int_op b);
        byte* stoi(byte* addr_ptr, int_op a, int_op b);
        byte* stof(byte* addr_ptr, int_op a, int_op b);

        byte* strstore(byte* addr_ptr, int_op reg, std::string s);

        byte* vec(byte* addr_ptr, int_op index);
        byte* vinsert(byte* addr_ptr, int_op vec, int_op src, int_op dst);
        byte* vpush(byte* addr_ptr, int_op vec, int_op src);
        byte* vpop(byte* addr_ptr, int_op vec, int_op dst, int_op pos);
        byte* vat(byte* addr_ptr, int_op vec, int_op dst, int_op at);
        byte* vlen(byte* addr_ptr, int_op vec, int_op reg);

        byte* lognot(byte* addr_ptr, int_op reg);
        byte* logand(byte* addr_ptr, int_op rega, int_op regb, int_op regr);
        byte* logor(byte* addr_ptr, int_op rega, int_op regb, int_op regr);

        byte* move(byte* addr_ptr, int_op a, int_op b);
        byte* copy(byte* addr_ptr, int_op a, int_op b);
        byte* ref(byte* addr_ptr, int_op a, int_op b);
        byte* swap(byte* addr_ptr, int_op a, int_op b);
        byte* free(byte* addr_ptr, int_op reg);
        byte* empty(byte* addr_ptr, int_op reg);
        byte* isnull(byte* addr_ptr, int_op a, int_op b);
        byte* ress(byte* addr_ptr, const std::string& a);
        byte* tmpri(byte* addr_ptr, int_op reg);
        byte* tmpro(byte* addr_ptr, int_op reg);

        byte* print(byte* addr_ptr, int_op reg);
        byte* echo(byte* addr_ptr, int_op reg);

        byte* clbind(byte* addr_ptr, int_op reg);
        byte* closure(byte* addr_ptr, const std::string& fn, int_op reg);
        byte* function(byte* addr_ptr, const std::string& fn, int_op reg);
        byte* fcall(byte* addr_ptr, int_op clsr, int_op ret);

        byte* frame(byte* addr_ptr, int_op a, int_op b);
        byte* param(byte* addr_ptr, int_op a, int_op b);
        byte* paref(byte* addr_ptr, int_op a, int_op b);
        byte* arg(byte* addr_ptr, int_op a, int_op b);
        byte* call(byte* addr_ptr, const std::string& fn_name, int_op reg);

        byte* jump(byte* addr_ptr, int addr);
        byte* branch(byte* addr_ptr, int_op regc, int addr_truth, int addr_false);

        byte* tryframe(byte* addr_ptr);
        byte* vmcatch(byte* addr_ptr, const std::string& type_name, const std::string& block_name);
        byte* pull(byte* addr_ptr, int_op regno);
        byte* vmtry(byte* addr_ptr, const std::string& block_name);
        byte* vmthrow(byte* addr_ptr, int_op regno);
        byte* leave(byte* addr_ptr);

        byte* eximport(byte* addr_ptr, const std::string& module_name);
        byte* excall(byte* addr_ptr, const std::string& fn_name, int_op reg);

        byte* end(byte* addr_ptr);
        byte* halt(byte* addr_ptr);
    }
}

#endif
