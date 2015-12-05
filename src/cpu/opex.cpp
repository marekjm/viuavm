#include <viua/bytecode/operand_types.h>
#include <viua/cpu/opex.h>


/** NOTICE
 *
 *  In this file there are lots of reinterpret_cast<>'s.
 *  It is in bad style, but C-style casts are even worse.
 *  We *know* that specified byte* really encode different datatypes.
 *  So, that leaves us with reinterpret_cast<> as it will allow the conversion.
 */

void viua::cpu::util::extractIntegerOperand(byte*& instruction_stream, bool& boolean, int& integer) {
    bool b = false;
    if (*reinterpret_cast<OperandType*>(instruction_stream) == OT_REGISTER_REFERENCE) {
        b = true;
    }
    boolean = b;
    pointer::inc<OperandType, byte>(instruction_stream);
    extractOperand<int>(instruction_stream, integer);
}

void viua::cpu::util::extractFloatingPointOperand(byte*& instruction_stream, float& fp) {
    extractOperand<float>(instruction_stream, fp);
}
