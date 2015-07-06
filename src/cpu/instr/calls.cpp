#include "../../types/boolean.h"
#include "../../support/pointer.h"
#include "../../exceptions.h"
#include "../cpu.h"
using namespace std;


byte* CPU::frame(byte* addr) {
    /** Create new frame for function calls.
     */
    int arguments, local_registers;
    bool arguments_ref = false, local_registers_ref = false;

    arguments_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    arguments = *((int*)addr);
    pointer::inc<int, byte>(addr);

    local_registers_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    local_registers = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (arguments_ref) {
        arguments = static_cast<Integer*>(fetch(arguments))->value();
    }
    if (local_registers_ref) {
        local_registers = static_cast<Integer*>(fetch(local_registers))->value();
    }

    requestNewFrame(arguments, local_registers);
    return addr;
}

byte* CPU::param(byte* addr) {
    /** Run param instruction.
     */
    int parameter_no_operand_index, object_operand_index;
    bool parameter_no_operand_ref = false, object_operand_ref = false;

    parameter_no_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    parameter_no_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    object_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    object_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (parameter_no_operand_ref) {
        parameter_no_operand_index = static_cast<Integer*>(fetch(parameter_no_operand_index))->value();
    }
    if (object_operand_ref) {
        object_operand_index = static_cast<Integer*>(fetch(object_operand_index))->value();
    }

    if (unsigned(parameter_no_operand_index) >= frame_new->args->size()) { throw new Exception("parameter register index out of bounds (greater than arguments set size) while adding parameter"); }
    frame_new->args->set(parameter_no_operand_index, fetch(object_operand_index)->copy());
    frame_new->args->clear(parameter_no_operand_index);

    return addr;
}

byte* CPU::paref(byte* addr) {
    /** Run paref instruction.
     */
    int parameter_no_operand_index, object_operand_index;
    bool parameter_no_operand_ref = false, object_operand_ref = false;

    parameter_no_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    parameter_no_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    object_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    object_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (parameter_no_operand_ref) {
        parameter_no_operand_index = static_cast<Integer*>(fetch(parameter_no_operand_index))->value();
    }
    if (object_operand_ref) {
        object_operand_index = static_cast<Integer*>(fetch(object_operand_index))->value();
    }

    if (unsigned(parameter_no_operand_index) >= frame_new->args->size()) { throw new Exception("parameter register index out of bounds (greater than arguments set size) while adding parameter"); }
    frame_new->args->set(parameter_no_operand_index, fetch(object_operand_index));
    frame_new->args->flag(parameter_no_operand_index, REFERENCE);

    return addr;
}

byte* CPU::arg(byte* addr) {
    /** Run arg instruction.
     */
    int parameter_no_operand_index, destination_register_index;
    bool parameter_no_operand_ref = false, destination_register_ref = false;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    parameter_no_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    parameter_no_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

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

byte* CPU::call(byte* addr) {
    /*  Run call instruction.
     */
    bool return_register_ref = *(bool*)addr;
    pointer::inc<bool, byte>(addr);

    int return_register_index = *(int*)addr;
    pointer::inc<int, byte>(addr);

    string call_name = string(addr);
    bool function_found = (function_addresses.count(call_name) or linked_functions.count(call_name));

    if (not function_found) {
        throw new Exception("call to undefined function: " + call_name);
    }

    byte* call_address = 0;
    if (function_addresses.count(call_name)) {
        call_address = bytecode+function_addresses.at(call_name);
        jump_base = bytecode;
    } else {
        call_address = linked_functions.at(call_name).second;
        jump_base = linked_modules.at(linked_functions.at(call_name).first).second;
    }
    addr += (call_name.size()+1);

    // save return address for frame
    byte* return_address = addr;

    if (frame_new == 0) {
        throw new Exception("function call without first_operand_index frame: use `frame 0' in source code if the function takes no parameters");
    }
    // set function name and return address
    frame_new->function_name = call_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = return_register_ref;
    frame_new->place_return_value_in = return_register_index;

    pushFrame();

    return call_address;
}

byte* CPU::end(byte* addr) {
    /*  Run end instruction.
     */
    if (frames.size() == 0) {
        throw new Exception("no frame on stack: nothing to end");
    }
    addr = frames.back()->ret_address();

    Type* returned = 0;
    bool returned_is_reference = false;
    int return_value_register = frames.back()->place_return_value_in;
    bool resolve_return_value_register = frames.back()->resolve_return_value_register;
    if (return_value_register != 0) {
        // we check in 0. register because it's reserved for return values
        if (uregset->at(0) == 0) {
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
        if (function_addresses.count(frames.back()->function_name)) {
            jump_base = bytecode;
        } else {
            jump_base = linked_modules.at(linked_functions.at(frames.back()->function_name).first).second;
        }
    }

    return addr;
}
