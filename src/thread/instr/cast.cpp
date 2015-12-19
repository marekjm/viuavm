#include <string>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/float.h>
#include <viua/types/byte.h>
#include <viua/types/string.h>
#include <viua/types/exception.h>
#include <viua/cpu/opex.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::itof(byte* addr) {
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    int convert_from = static_cast<Integer*>(viua::operand::extract(addr)->resolve(this))->value();
    place(target, new Float(static_cast<float>(convert_from)));

    return addr;
}

byte* Thread::ftoi(byte* addr) {
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    float convert_from = static_cast<Float*>(viua::operand::extract(addr)->resolve(this))->value();
    place(target, new Integer(static_cast<int>(convert_from)));

    return addr;
}

byte* Thread::stoi(byte* addr) {
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    int result_integer = 0;
    string supplied_string = static_cast<String*>(viua::operand::extract(addr)->resolve(this))->value();
    try {
        result_integer = std::stoi(supplied_string);
    } catch (const std::out_of_range& e) {
        throw new Exception("out of range: " + supplied_string);
    } catch (const std::invalid_argument& e) {
        throw new Exception("invalid argument: " + supplied_string);
    }

    place(target, new Integer(result_integer));

    return addr;
}

byte* Thread::stof(byte* addr) {
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    string supplied_string = static_cast<String*>(viua::operand::extract(addr)->resolve(this))->value();
    double convert_from = std::stod(supplied_string);
    place(target, new Float(static_cast<float>(convert_from)));

    return addr;
}
