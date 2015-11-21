#include <viua/types/boolean.h>
#include <viua/types/reference.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::frame(byte* addr) {
    /** Create new frame for function calls.
     */
    int arguments, local_registers;
    bool arguments_ref = false, local_registers_ref = false;

    viua::cpu::util::extractIntegerOperand(addr, arguments_ref, arguments);
    viua::cpu::util::extractIntegerOperand(addr, local_registers_ref, local_registers);

    if (arguments_ref) {
        arguments = static_cast<Integer*>(fetch(arguments))->value();
    }
    if (local_registers_ref) {
        local_registers = static_cast<Integer*>(fetch(local_registers))->value();
    }

    requestNewFrame(arguments, local_registers);
    return addr;
}

byte* Thread::param(byte* addr) {
    /** Run param instruction.
     */
    int parameter_no_operand_index, object_operand_index;
    bool parameter_no_operand_ref = false, object_operand_ref = false;

    viua::cpu::util::extractIntegerOperand(addr, parameter_no_operand_ref, parameter_no_operand_index);
    viua::cpu::util::extractIntegerOperand(addr, object_operand_ref, object_operand_index);

    if (parameter_no_operand_ref) {
        parameter_no_operand_index = static_cast<Integer*>(fetch(parameter_no_operand_index))->value();
    }
    if (object_operand_ref) {
        object_operand_index = static_cast<Integer*>(fetch(object_operand_index))->value();
    }

    if (unsigned(parameter_no_operand_index) >= frame_new->args->size()) { throw new Exception("parameter register index out of bounds (greater than arguments set size) while adding parameter"); }
    frame_new->args->set(parameter_no_operand_index, fetch(object_operand_index));
    frame_new->args->clear(parameter_no_operand_index);

    return addr;
}

byte* Thread::paref(byte* addr) {
    /** Run paref instruction.
     */
    int parameter_no_operand_index, object_operand_index;
    bool parameter_no_operand_ref = false, object_operand_ref = false;

    viua::cpu::util::extractIntegerOperand(addr, parameter_no_operand_ref, parameter_no_operand_index);
    viua::cpu::util::extractIntegerOperand(addr, object_operand_ref, object_operand_index);

    if (parameter_no_operand_ref) {
        parameter_no_operand_index = static_cast<Integer*>(fetch(parameter_no_operand_index))->value();
    }
    if (object_operand_ref) {
        object_operand_index = static_cast<Integer*>(fetch(object_operand_index))->value();
    }

    if (unsigned(parameter_no_operand_index) >= frame_new->args->size()) {
        throw new Exception("parameter register index out of bounds (greater than arguments set size) while adding parameter");
    }

    Type* object = uregset->at(object_operand_index);
    Reference* rf = dynamic_cast<Reference*>(object);
    if (rf == nullptr) {
        rf = new Reference(object);
        uregset->empty(object_operand_index);
        uregset->set(object_operand_index, rf);
    }
    frame_new->args->set(parameter_no_operand_index, rf);

    return addr;
}

byte* Thread::arg(byte* addr) {
    /** Run arg instruction.
     */
    int parameter_no_operand_index, destination_register_index;
    bool parameter_no_operand_ref = false, destination_register_ref = false;

    viua::cpu::util::extractIntegerOperand(addr, destination_register_ref, destination_register_index);
    viua::cpu::util::extractIntegerOperand(addr, parameter_no_operand_ref, parameter_no_operand_index);

    if (parameter_no_operand_ref) {
        parameter_no_operand_index = static_cast<Integer*>(fetch(parameter_no_operand_index))->value();
    }
    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    if (unsigned(parameter_no_operand_index) >= frames.back()->args->size()) {
        ostringstream oss;
        oss << "invalid read: read from argument register out of bounds: " << parameter_no_operand_index;
        throw new Exception(oss.str());
    }

    if (frames.back()->args->isflagged(parameter_no_operand_index, REFERENCE)) {
        uregset->set(destination_register_index, frames.back()->args->get(parameter_no_operand_index));
    } else {
        uregset->set(destination_register_index, frames.back()->args->get(parameter_no_operand_index)->copy());
    }
    uregset->setmask(destination_register_index, frames.back()->args->getmask(parameter_no_operand_index));  // set correct mask

    return addr;
}

byte* Thread::argc(byte* addr) {
    /** Run arg instruction.
     */
    int destination_register_index;
    bool destination_register_ref = false;

    viua::cpu::util::extractIntegerOperand(addr, destination_register_ref, destination_register_index);

    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    uregset->set(destination_register_index, new Integer(static_cast<int>(frames.back()->args->size())));

    return addr;
}

byte* Thread::call(byte* addr) {
    /*  Run call instruction.
     */
    bool return_register_ref = false;
    int return_register_index = 0;
    viua::cpu::util::extractIntegerOperand(addr, return_register_ref, return_register_index);

    string call_name = string(addr);

    bool is_native = (cpu->function_addresses.count(call_name) or cpu->linked_functions.count(call_name));
    bool is_foreign = cpu->foreign_functions.count(call_name);

    if (not (is_native or is_foreign)) {
        throw new Exception("call to undefined function: " + call_name);
    }

    auto caller = (is_native ? &Thread::callNative : &Thread::callForeign);
    return (this->*caller)(addr, call_name, return_register_ref, return_register_index, "");
}

byte* Thread::end(byte* addr) {
    /*  Run end instruction.
     */
    if (frames.size() == 0) {
        throw new Exception("no frame on stack: nothing to end");
    }
    addr = frames.back()->ret_address();

    Type* returned = nullptr;
    bool returned_is_reference = false;
    int return_value_register = frames.back()->place_return_value_in;
    bool resolve_return_value_register = frames.back()->resolve_return_value_register;
    if (return_value_register != 0) {
        // we check in 0. register because it's reserved for return values
        if (uregset->at(0) == nullptr) {
            throw new Exception("return value requested by frame but function did not set return register");
        }
        if (uregset->isflagged(0, REFERENCE)) {
            returned = uregset->get(0);
            returned_is_reference = true;
        } else {
            returned = uregset->get(0)->copy();
        }
    }

    dropFrame();

    // place return value
    if (returned and frames.size() > 0) {
        if (resolve_return_value_register) {
            return_value_register = static_cast<Integer*>(fetch(return_value_register))->value();
        }
        place(return_value_register, returned);
        if (returned_is_reference) {
            uregset->flag(return_value_register, REFERENCE);
        }
    }

    if (frames.size() > 0) {
        if (cpu->function_addresses.count(frames.back()->function_name)) {
            jump_base = cpu->bytecode;
        } else {
            jump_base = cpu->linked_modules.at(cpu->linked_functions.at(frames.back()->function_name).first).second;
        }
    }

    return addr;
}
