#include <viua/support/pointer.h>
#include <viua/cpu/opex.h>


/** NOTICE
 *
 *  In this file there are lots of reinterpret_cast<>'s.
 *  It is in bad style, but C-style casts are even worse.
 *  We *know* that specified byte* really encode different datatypes.
 *  So, that leaves us with reinterpret_cast<> as it will allow the conversion.
 */

void viua::cpu::util::extractIntegerOperand(byte*& instruction_stream, bool& boolean, int& integer) {
    bool ex_bool = false;
    int ex_int = 0;

    ex_bool = *(reinterpret_cast<bool*>(instruction_stream));
    pointer::inc<bool, byte>(instruction_stream);

    ex_int = *(reinterpret_cast<int*>(instruction_stream));
    pointer::inc<int, byte>(instruction_stream);

    boolean = ex_bool;
    integer = ex_int;
}
