#include <iostream>
#include <algorithm>
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/maps.h>
#include <viua/types/integer.h>
#include <viua/types/exception.h>
#include <viua/types/reference.h>
#include <viua/thread.h>
#include <viua/cpu/cpu.h>
using namespace std;


Type* Thread::fetch(unsigned index) const {
    /*  Return pointer to object at given register.
     *  This method safeguards against reaching for out-of-bounds registers and
     *  reading from an empty register.
     *
     *  :params:
     *
     *  index:int   - index of a register to fetch
     */
    Type* object = uregset->get(index);
    if (dynamic_cast<Reference*>(object)) {
        object = static_cast<Reference*>(object)->pointsTo();
    }
    return object;
}
void Thread::place(unsigned index, Type* obj) {
    /** Place an object in register with given index.
     *
     *  Before placing an object in register, a check is preformed if the register is empty.
     *  If not - the `Type` previously stored in it is destroyed.
     *
     */
    Type* old_ref_ptr = (hasrefs(index) ? uregset->at(index) : nullptr);
    uregset->set(index, obj);

    // update references *if, and only if* the register being set has references and
    // is *not marked a reference* itself, i.e. is the origin register
    if (old_ref_ptr and not uregset->isflagged(index, REFERENCE)) {
        updaterefs(old_ref_ptr, obj);
    }
}
void Thread::updaterefs(Type* before, Type* now) {
    /** This method updates references to a given address present in registers.
     *  It swaps old address for the new one in every register that points to the old address.
     *
     *  There is no need to delete old object in this function, as it will be deleted as soon as
     *  it is replaced in the origin register (i.e. the register that holds the original pointer to
     *  the object - the one from which all references had been derived).
     */
    // FIXME: this function should update references in all registersets
    for (unsigned i = 0; i < uregset->size(); ++i) {
        if (uregset->at(i) == before) {
            if (debug) {
                cout << "\nCPU: updating reference address in register " << i << hex << ": " << before << " -> " << now << dec << endl;
            }
            mask_t had_mask = uregset->getmask(i);
            uregset->empty(i);
            uregset->set(i, now);
            uregset->setmask(i, had_mask);
        }
    }
}
bool Thread::hasrefs(unsigned index) {
    /** This method checks if object at a given address exists as a reference in another register.
     */
    bool has = false;
    // FIXME: this should check for references in every register set; gonna be slow, isn't it?
    for (unsigned i = 0; i < uregset->size(); ++i) {
        if (i == index) continue;
        if (uregset->at(i) == uregset->at(index)) {
            has = true;
            break;
        }
    }
    return has;
}
void Thread::ensureStaticRegisters(string function_name) {
    /** Makes sure that static register set for requested function is initialized.
     */
    try {
        static_registers.at(function_name);
    } catch (const std::out_of_range& e) {
        // FIXME: amount of static registers should be customizable
        static_registers[function_name] = new RegisterSet(16);
    }
}

Frame* Thread::requestNewFrame(int arguments_size, int registers_size) {
    /** Request new frame to be prepared.
     *
     *  Creates new frame if the new-frame hook is empty.
     *  Throws an exception otherwise.
     *  Returns pointer to the newly created frame.
     */
    if (frame_new != nullptr) { throw "requested new frame while last one is unused"; }
    return (frame_new = new Frame(nullptr, arguments_size, registers_size));
}
void Thread::pushFrame() {
    /** Pushes new frame to be the current (top-most) one.
     */
    if (frames.size() > MAX_STACK_SIZE) {
        ostringstream oss;
        oss << "stack size (" << MAX_STACK_SIZE << ") exceeded with call to '" << frame_new->function_name << '/' << frame_new->args->size() << '\'';
        throw new Exception(oss.str());
    }

    uregset = frame_new->regset;
    // FIXME: remove this print
    //cout << "\npushing new frame on stack: " << hex << frame_new << dec << " (for function: " << frame_new->function_name << ')' << endl;
    if (find(frames.begin(), frames.end(), frame_new) != frames.end()) {
        ostringstream oss;
        oss << "stack corruption: frame " << hex << frame_new << dec << " for function " << frame_new->function_name << '/' << frame_new->args->size() << " pushed more than once";
        throw oss.str();
    }
    frames.push_back(frame_new);
    frame_new = nullptr;
}
void Thread::dropFrame() {
    /** Drops top-most frame from call stack.
     */
    delete frames.back();
    frames.pop_back();

    if (frames.size()) {
        uregset = frames.back()->regset;
    } else {
        uregset = regset;
    }
}

byte* Thread::callNative(byte* addr, const string& call_name, const bool& return_ref, const int& return_index, const string& real_call_name) {
    byte* call_address = nullptr;
    if (cpu->function_addresses.count(call_name)) {
        call_address = cpu->bytecode+(cpu->function_addresses.at(call_name));
        jump_base = cpu->bytecode;
    } else {
        call_address = cpu->linked_functions.at(call_name).second;
        jump_base = cpu->linked_modules.at(cpu->linked_functions.at(call_name).first).second;
    }
    if (real_call_name.size()) {
        addr += (real_call_name.size()+1);
    } else {
        addr += (call_name.size()+1);
    }


    // save return address for frame
    byte* return_address = addr;

    if (frame_new == nullptr) {
        throw new Exception("function call without first_operand_index frame: use `frame 0' in source code if the function takes no parameters");
    }
    // set function name and return address
    frame_new->function_name = call_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = return_ref;
    frame_new->place_return_value_in = return_index;

    pushFrame();

    return call_address;
}
byte* Thread::callForeign(byte* addr, const string& call_name, const bool& return_ref, const int& return_index, const string& real_call_name) {
    if (real_call_name.size()) {
        addr += (real_call_name.size()+1);
    } else {
        addr += (call_name.size()+1);
    }

    // save return address for frame
    byte* return_address = addr;

    if (frame_new == nullptr) {
        throw new Exception("external function call without a frame: use `frame 0' in source code if the function takes no parameters");
    }
    // set function name and return address
    frame_new->function_name = call_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = return_ref;
    frame_new->place_return_value_in = return_index;

    Frame* frame = frame_new;

    pushFrame();

    if (cpu->foreign_functions.count(call_name) == 0) {
        throw new Exception("call to unregistered external function: " + call_name);
    }

    /* FIXME: second parameter should be a pointer to static registers or
     *        0 if function does not have static registers registered
     * FIXME: should external functions always have static registers allocated?
     */
    ExternalFunction* callback = cpu->foreign_functions.at(call_name);
    (*callback)(frame, nullptr, regset);

    // FIXME: woohoo! segfault!
    Type* returned = nullptr;
    bool returned_is_reference = false;
    int return_value_register = frames.back()->place_return_value_in;
    bool resolve_return_value_register = frames.back()->resolve_return_value_register;
    if (return_value_register != 0) {
        // we check in 0. register because it's reserved for return values
        if (uregset->at(0) == nullptr) {
            throw new Exception("return value requested by frame but external function did not set return register");
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

    return return_address;
}
byte* Thread::callForeignMethod(byte* addr, Type* object, const string& call_name, const bool& return_ref, const int& return_index, const string& real_call_name) {
    if (real_call_name.size()) {
        addr += (real_call_name.size()+1);
    } else {
        addr += (call_name.size()+1);
    }

    // save return address for frame
    byte* return_address = addr;

    if (frame_new == nullptr) {
        throw new Exception("foreign method call without a frame");
    }
    // set function name and return address
    frame_new->function_name = call_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = return_ref;
    frame_new->place_return_value_in = return_index;

    Frame* frame = frame_new;

    pushFrame();

    if (cpu->foreign_methods.count(call_name) == 0) {
        throw new Exception("call to unregistered foreign method: " + call_name);
    }

    Reference* rf = nullptr;
    if ((rf = dynamic_cast<Reference*>(object))) {
        object = rf->pointsTo();
    }

    try {
        // FIXME: supply static and global registers to foreign functions
        cpu->foreign_methods.at(call_name)(object, frame, nullptr, nullptr);
    } catch (const std::out_of_range& e) {
        throw new Exception(e.what());
    }

    // FIXME: woohoo! segfault!
    Type* returned = nullptr;
    bool returned_is_reference = false;
    int return_value_register = frames.back()->place_return_value_in;
    bool resolve_return_value_register = frames.back()->resolve_return_value_register;
    if (return_value_register != 0) {
        // we check in 0. register because it's reserved for return values
        if (uregset->at(0) == nullptr) {
            throw new Exception("return value requested by frame but foreign method did not set return register");
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

    return return_address;
}

byte* Thread::xtick() {
    /** Perform a *tick*, i.e. run a single CPU instruction.
     *
     *  Returns pointer to next instruction upon correct execution.
     *  Returns null pointer upon error.
     */
    bool halt = false;
    byte* previous_instruction_pointer = instruction_pointer;
    ++instruction_counter;

    try {
        instruction_pointer = dispatch(instruction_pointer);
    } catch (Exception* e) {
        /* All machine-thrown exceptions are passed back to user code.
         * This is much easier than checking for erroneous conditions and
         * terminating functions conditionally, instead - machine just throws Exception objects which
         * are then caught here.
         *
         * If user code cannot deal with them (i.e. did not register a catcher block) they will terminate execution later.
         */
        thrown = e;
    } catch (const HaltException& e) {
        halt = true;
    } catch (const char* e) {
        thrown = new Exception(e);
    }

    if (halt or frames.size() == 0) { return nullptr; }

    /*  Machine should halt execution if the instruction pointer exceeds bytecode size and
     *  top frame is for local function.
     *  For dynamically linked functions address will not be in bytecode size range.
     */
    Frame* top_frame = (frames.size() ? frames.back() : nullptr);
    TryFrame* top_tryframe = (tryframes.size() ? tryframes.back() : nullptr);
    bool is_current_function_dynamic = cpu->linked_functions.count(top_frame != nullptr ? top_frame->function_name : "");
    bool is_current_block_dynamic = cpu->linked_blocks.count(top_tryframe != nullptr ? top_tryframe->block_name : "");
    if (instruction_pointer >= (cpu->bytecode+cpu->bytecode_size) and not (is_current_function_dynamic or is_current_block_dynamic)) {
        return_code = 1;
        return_exception = "InvalidBytecodeAddress";
        return_message = string("instruction address out of bounds");
        return nullptr;
    }

    /*  Machine should halt execution if previous instruction pointer is the same as current one as
     *  it means that the execution flow is corrupted.
     *
     *  However, execution *should not* be halted if:
     *      - the offending opcode is END (as this may indicate exiting recursive function),
     *      - an object has been thrown, as the instruction pointer will be adjusted by
     *        catchers or execution will be halted on unhandled types,
     */
    if (instruction_pointer == previous_instruction_pointer and OPCODE(*instruction_pointer) != END and thrown == nullptr) {
        return_code = 2;
        ostringstream oss;
        return_exception = "InstructionUnchanged";
        oss << "instruction pointer did not change, possibly endless loop\n";
        oss << "note: instruction index was " << (cpu->instruction_pointer-cpu->bytecode) << " and the opcode was '" << OP_NAMES.at(OPCODE(*instruction_pointer)) << "'";
        if (OPCODE(*instruction_pointer) == CALL) {
            oss << '\n';
            oss << "note: this was caused by 'call' opcode immediately calling itself\n"
                << "      such situation may have several sources, e.g. empty function definition or\n"
                << "      a function which calls itself in its first instruction";
        }
        return_message = oss.str();
        return nullptr;
    }

    TryFrame* tframe;
    // WARNING!
    // This is a temporary hack for more fine-grained exception handling
    if (thrown != nullptr and thrown->type() == "Exception") {
        string exception_detailed_type = static_cast<Exception*>(thrown)->etype();
        for (long unsigned i = tryframes.size(); i > 0; --i) {
            tframe = tryframes[(i-1)];
            if (tframe->catchers.count(exception_detailed_type)) {
                instruction_pointer = tframe->catchers.at(exception_detailed_type)->block_address;

                caught = thrown;
                thrown = nullptr;

                break;
            }
        }
    }

    if (thrown != nullptr) {
        for (long unsigned i = tryframes.size(); i > 0; --i) {
            tframe = tryframes[(i-1)];
            string handler_found_for_type = thrown->type();
            bool handler_found = tframe->catchers.count(handler_found_for_type);

            if ((not handler_found) and cpu->typesystem.count(handler_found_for_type)) {
                vector<string> types_to_check = cpu->inheritanceChainOf(handler_found_for_type);
                for (unsigned j = 0; j < types_to_check.size(); ++j) {
                    if (tframe->catchers.count(types_to_check[j])) {
                        handler_found = true;
                        handler_found_for_type = types_to_check[j];
                        break;
                    }
                }
            }

            if (handler_found) {
                instruction_pointer = tframe->catchers.at(handler_found_for_type)->block_address;

                unsigned distance = 0;
                for (long unsigned j = (frames.size()-1); j > 1; --j) {
                    if (frames[j] == tframe->associated_frame) {
                        break;
                    }
                    ++distance;
                }
                for (unsigned j = 0; j < distance; ++j) {
                    dropFrame();
                }

                while (tryframes.back() != tframe) {
                    delete tryframes.back();
                    tryframes.pop_back();
                }

                caught = thrown;
                thrown = nullptr;

                break;
            }
        }
    }
    if (thrown != nullptr) {
        return_code = 1;
        return_exception = thrown->type();
        return_message = thrown->repr();
        return nullptr;
    }

    return instruction_pointer;
}

byte* Thread::tick() {
    bool halt = false;

    byte* previous_instruction_pointer = instruction_pointer;
    ++instruction_counter;

    try {
        /* cout << "pre:  " << reinterpret_cast<long unsigned>(instruction_pointer) << " " << OP_NAMES.at(OPCODE(*instruction_pointer)) << endl; */
        instruction_pointer = dispatch(instruction_pointer);
        /* cout << "post: " << reinterpret_cast<long unsigned>(instruction_pointer) << " " << OP_NAMES.at(OPCODE(*instruction_pointer)) << endl; */
    } catch (Exception* e) {
        /* All machine-thrown exceptions are passed back to user code.
         * This is much easier than checking for erroneous conditions and
         * terminating functions conditionally, instead - machine just throws Exception objects which
         * are then caught here.
         *
         * If user code cannot deal with them (i.e. did not register a catcher block) they will terminate execution later.
         */
        // FIXME: remove the print
        //cout << "event 0" << endl;
        thrown = e;
    } catch (const HaltException& e) {
        // FIXME: remove the print
        //cout << "event 1" << endl;
        halt = true;
    } catch (const char* e) {
        // FIXME: remove the print
        //cout << "event 2 " << e << endl;
        thrown = new Exception(e);
    }

    if (halt or frames.size() == 0) {
        // FIXME: remove the print
        //cout << "halted" << endl;
        return nullptr;
    }

    TryFrame* tframe;

    if (thrown != nullptr and thrown->type() == "Exception") {
        string exception_detailed_type = static_cast<Exception*>(thrown)->etype();
        for (long unsigned i = tryframes.size(); i > 0; --i) {
            tframe = tryframes[(i-1)];
            if (tframe->catchers.count(exception_detailed_type)) {
                instruction_pointer = tframe->catchers.at(exception_detailed_type)->block_address;

                caught = thrown;
                thrown = nullptr;

                break;
            }
        }
    }

    if (thrown != nullptr) {
        for (long unsigned i = tryframes.size(); i > 0; --i) {
            tframe = tryframes[(i-1)];
            string handler_found_for_type = thrown->type();
            bool handler_found = tframe->catchers.count(handler_found_for_type);

            // FIXME: mutex
            if ((not handler_found) and cpu->typesystem.count(handler_found_for_type)) {
                vector<string> types_to_check = cpu->inheritanceChainOf(handler_found_for_type);
                for (unsigned j = 0; j < types_to_check.size(); ++j) {
                    if (tframe->catchers.count(types_to_check[j])) {
                        handler_found = true;
                        handler_found_for_type = types_to_check[j];
                        break;
                    }
                }
            }

            if (handler_found) {
                instruction_pointer = tframe->catchers.at(handler_found_for_type)->block_address;

                unsigned distance = 0;
                for (long unsigned j = (frames.size()-1); j > 1; --j) {
                    if (frames[j] == tframe->associated_frame) {
                        break;
                    }
                    ++distance;
                }
                for (unsigned j = 0; j < distance; ++j) {
                    dropFrame();
                }

                while (tryframes.back() != tframe) {
                    delete tryframes.back();
                    tryframes.pop_back();
                }

                caught = thrown;
                thrown = nullptr;

                break;
            }
        }
    }

    if (thrown != nullptr) {
        // FIXME: remove the print
        //cout << "unhandled exception" << endl;
        has_unhandled_exception = true;
        return nullptr;
    }

    return instruction_pointer;
}

void Thread::iframe(Frame* frm, unsigned r) {
    // FIXME: remove this placeholder function
}

byte* Thread::begin() {
    return (instruction_pointer = (cpu->bytecode + cpu->function_addresses.at(frames[0]->function_name)));
}

Thread::~Thread() {
    while (frames.size()) {
        delete frames.back();
        frames.pop_back();
    }

    decltype(static_registers)::iterator sr = static_registers.begin();
    while (sr != static_registers.end()) {
        auto rkey = sr->first;
        auto rset = sr->second;
        ++sr;
        static_registers.erase(rkey);
        delete rset;
    }
}
