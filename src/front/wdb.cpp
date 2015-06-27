#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include "../../lib/linenoise/linenoise.h"
#include "../version.h"
#include "../bytecode/maps.h"
#include "../support/string.h"
#include "../support/pointer.h"
#include "../types/integer.h"
#include "../types/closure.h"
#include "../loader.h"
#define AS_DEBUG_HEADER 1
#include "../cpu/cpu.h"
#include "../program.h"
#include "../cg/disassembler/disassembler.h"
#include "../printutils.h"
#include "../include/module.h"
using namespace std;


const char* NOTE_LOADED_ASM = "note: seems like you have loaded an .asm file which cannot be run on CPU without prior compilation";
const char* RC_FILENAME = "/.viuavm.db.rc";
const char* DEBUGGER_COMMAND_HISTORY = "/.viuavmdb_history";


const vector<string> DEBUGGER_COMMANDS = {
    "cpu.",
    "cpu.init",
    "cpu.run",
    "cpu.tick",
    "cpu.jump",
    "cpu.unpause",
    "cpu.resume",
    "cpu.unfinish",
    "cpu.counter",
    "breakpoint.",
    "breakpoint.set.",
    "breakpoint.set.at",
    "breakpoint.set.on.",
    "breakpoint.set.on.opcode",
    "breakpoint.set.on.function",
    "watch.",
    "watch.register.",
    "watch.register.local.",
    "watch.register.local.write",
    "watch.register.local.read",
    "watch.register.global.",
    "watch.register.global.write",
    "watch.register.global.read",
    "watch.register.static.",
    "watch.register.static.write",
    "watch.register.static.read",
    "register.",
    "register.show",
    "register.local.",
    "register.local.show",
    "register.global.",
    "register.global.show",
    "register.static.",
    "register.static.show",
    "arguments.show",
    "print.ahead",
    "trace",
    "stack.",
    "stack.trace",
    "stack.trace.show",
    "stack.frame.show",
    "loader.",
    "loader.function.map",
    "loader.function.map.show",
    "loader.block.map",
    "loader.block.map.show",
    "loader.extern.function.map",
    "loader.extern.function.map.show",
    "help",
    "quit",
};


// MISC FLAGS
bool SHOW_HELP = false;
bool SHOW_VERSION = false;
bool VERBOSE = false;


OPCODE printInstruction(const CPU& cpu) {
    byte* iptr = cpu.instruction_pointer;

    string instruction;
    unsigned size;
    tie(instruction, size) = disassembler::instruction(iptr);

    cout << "byte " << (iptr-cpu.bytecode) << hex << " (0x" << (iptr-cpu.bytecode) << ") ";
    cout << "at 0x" << long(iptr) << dec << ": ";
    cout << instruction << endl;

    return OPCODE(*iptr);
}

void printRegisters(const vector<string>& indexes, RegisterSet* regset) {
    /** Print selected registers.
     *
     *  This function handles pretty printing of register contents.
     *  While printed, indexes are bound-checked.
     */
    vector<unsigned> to_display = {};
    for (unsigned i = 0; i < indexes.size(); ++i) {
        if (not str::isnum(indexes[i])) {
            cout << "error: invalid operand, expected integer" << endl;
            continue;
        }
        to_display.push_back(stoi(indexes[i]));
    }
    if (indexes.size() == 0) {
        for (unsigned i = 0; i < regset->size(); ++i) {
            to_display.push_back(i);
        }
    }

    unsigned index;
    for (unsigned i = 0; i < to_display.size(); ++i) {
        index = to_display[i];
        if (index >= regset->size()) {
            cout << "warn: register index out of range: " << index << endl;
            continue;
        }
        cout << "-- " << index << " --";
        Type* object = regset->at(index);
        if (object) {
            cout << '\n';
            cout << "  pointer:       " << hex << object << dec << endl;
            cout << "  reference:     " << (regset->isflagged(index, REFERENCE) ? "true" : "false") << '\n';
            cout << "  copy-on-write: " << (regset->isflagged(index, COPY_ON_WRITE) ? "true" : "false") << '\n';
            cout << "  keep:          " << (regset->isflagged(index, KEEP) ? "true" : "false") << '\n';
            cout << "  to-be bound:   " << (regset->isflagged(index, BIND) ? "true" : "false") << '\n';
            cout << "  bound:         " << (regset->isflagged(index, BOUND) ? "true" : "false") << '\n';
            cout << "  object type:   " << object->type() << '\n';
            cout << "  value:         " << object->repr() << '\n';
        } else {
            cout << " [empty]" << endl;
        }
    }
}


struct State {
    // breakpoints
    vector<byte*> breakpoints_byte;
    vector<string> breakpoints_opcode;
    vector<string> breakpoints_function;

    // watchpoints (FIXME)
    map<string, vector<int> > watch_register_local_write;
    map<string, vector<int> > watch_register_local_read;
    map<string, vector<int> > watch_register_static_write;
    map<string, vector<int> > watch_register_static_read;
    vector<int> watch_register_global_write;
    vector<int> watch_register_global_read;

    // init/pause/finish/quit
    bool initialised = false;
    bool paused = false;
    bool finished = false;
    bool quit = false;

    // exception state
    bool exception_raised = false;
    string exception_type, exception_message;

    // tick control
    int ticks_left = 0;
    int autoresumes = 0;

    State():
        breakpoints_byte({}), breakpoints_opcode({}), breakpoints_function({}),
        initialised(false), paused(false), finished(false), quit(false),
        exception_raised(false), exception_type(""), exception_message(""),
        ticks_left(0), autoresumes(0)
    {}
};


tuple<bool, string> if_breakpoint_byte(CPU& cpu, vector<byte*>& breakpoints_byte) {
    bool pause = false;
    ostringstream reason;
    reason.str("");

    if (find(breakpoints_byte.begin(), breakpoints_byte.end(), cpu.instruction_pointer) != breakpoints_byte.end()) {
        reason << "info: execution paused by byte breakpoint: " << cpu.instruction_pointer;
        pause = true;
    }

    return tuple<bool, string>(pause, reason.str());
}
tuple<bool, string> if_breakpoint_opcode(CPU& cpu, vector<string>& breakpoints_opcode) {
    bool pause = false;
    ostringstream reason;
    reason.str("");

    string op_name = OP_NAMES.at(OPCODE(*cpu.instruction_pointer));

    if (find(breakpoints_opcode.begin(), breakpoints_opcode.end(), op_name) != breakpoints_opcode.end()) {
        reason << "info: execution halted by opcode breakpoint: " << op_name;
        pause = true;
    }

    return tuple<bool, string>(pause, reason.str());
}
tuple<bool, string> if_breakpoint_function(CPU& cpu, vector<string>& breakpoints_function) {
    bool pause = false;
    ostringstream reason;
    reason.str("");

    string op_name = OP_NAMES.at(OPCODE(*cpu.instruction_pointer));

    if (op_name == "call") {
        string function_name = string(cpu.instruction_pointer+1);
        if (find(breakpoints_function.begin(), breakpoints_function.end(), function_name) != breakpoints_function.end()) {
            reason << "info: execution halted by function breakpoint: " << function_name;
            pause = true;
        }
    }

    return tuple<bool, string>(pause, reason.str());
}

tuple<bool, string> if_watchpoint_local_register_write(CPU& cpu, const State& state) {
    /** Determine whether the instruction at instruction pointer should trigger a watchpoint.
     */
    bool writing_instruction = true;
    OPCODE opcode = OPCODE(*cpu.instruction_pointer);
    if (opcode == NOP or
        opcode == NOP or
        opcode == RESS or
        opcode == TMPRI or
        opcode == PRINT or
        opcode == ECHO or
        opcode == CLFRAME or
        opcode == CLCALL or
        opcode == FRAME or
        opcode == PARAM or
        opcode == PAREF or
        opcode == CALL or
        opcode == JUMP or
        opcode == BRANCH or
        opcode == END or
        opcode == HALT
       ) {
        writing_instruction = false;
    }
    int register_index[2] = {-1, -1};
    int writes_to = 0;
    byte* register_index_ptr = (cpu.instruction_pointer+1);

    if (opcode == IZERO or
        opcode == ISTORE or
        opcode == IINC or
        opcode == IDEC or
        opcode == FSTORE or
        opcode == BSTORE or
        opcode == STRSTORE or
        opcode == VEC or
        opcode == VINSERT or
        opcode == VPUSH or
        opcode == BOOL or
        opcode == NOT or
        opcode == FREE or
        opcode == EMPTY or
        opcode == TMPRO
       ) {
        register_index[0] = *(int*)(register_index_ptr+1);
        writes_to = 1;
    } else if (opcode == ITOF or
               opcode == FTOI or
               opcode == STOI or
               opcode == STOF or
               opcode == VLEN or
               opcode == MOVE or
               opcode == COPY or
               opcode == REF or
               opcode == ISNULL or
               opcode == ARG
            ) {
        register_index[0] = *((int*)(register_index_ptr+2)+1);
        writes_to = 1;
    } else if (opcode == IADD or
               opcode == ISUB or
               opcode == IMUL or
               opcode == IDIV or
               opcode == ILT or
               opcode == ILTE or
               opcode == IGT or
               opcode == IGTE or
               opcode == IEQ or
               opcode == FADD or
               opcode == FSUB or
               opcode == FMUL or
               opcode == FDIV or
               opcode == FLT or
               opcode == FLTE or
               opcode == FGT or
               opcode == FGTE or
               opcode == FEQ or
               opcode == BADD or
               opcode == BSUB or
               opcode == BLT or
               opcode == BLTE or
               opcode == BGT or
               opcode == BGTE or
               opcode == BEQ or
               opcode == VAT or
               opcode == AND or
               opcode == OR
               ) {
        register_index[0] = *((int*)(register_index_ptr+3)+2);
        writes_to = 1;
    } else if (opcode == VPOP or opcode == SWAP) {
        register_index[0] = *((int*)(++register_index_ptr)++);
        register_index[1] = *((int*)(++register_index_ptr)++);
        writes_to = 2;
    }

    bool pause = false;
    ostringstream reason;
    reason.str("");

    if (writing_instruction) {
        auto search = state.watch_register_local_write.find(cpu.trace().back()->function_name);
        if (search != state.watch_register_local_write.end()) {
            for (int i = 0; i < writes_to; ++i) {
                if (find(search->second.begin(), search->second.end(), register_index[i]) != search->second.end()) {
                    pause = true;
                    reason << "info: execution halted by local register write watchpoint: " << register_index[i];
                }
            }
        }
    }

    return tuple<bool, string>(pause, reason.str());
}


bool command_verify(string& command, vector<string>& operands, const CPU& cpu, const State& state) {
    /** Basic command verification.
     *
     *  This function check only for the most obvious errors and
     *  syntax violations.
     */
    bool verified = true;

    if (command == "") {
        // do nothing...
    } else if (command == "conf.set") {
        if (not operands.size()) {
            cout << "error: missing operands: <key> [value]" << endl;
            verified = false;
        } else if (operands[0] == "cpu.debug") {
            if (operands.size() != 1 and (operands.size() > 1 and not (operands[1] == "true" or operands[1] == "false"))) {
                cout << "error: invalid operand, expected 'true' of 'false'" << endl;
                verified = false;
            }
            if (operands.size() == 1) {
                operands.push_back("true");
            }
        } else {
            cout << "error: invalid setting" << endl;
            verified = false;
        }
    } else if (command == "conf.get") {
    } else if (command == "conf.load") {
        cout << "FIXME/TODO" << endl;
    } else if (command == "conf.load.default") {
        cout << "FIXME/TODO" << endl;
    } else if (command == "conf.dump") {
        cout << "FIXME/TODO" << endl;
    } else if (command == "breakpoint.set.at") {
        if (not operands.size()) {
            cout << "warn: requires integer operand(s)" << endl;
            verified = false;
        }
    } else if (command == "breakpoint.set.on.opcode") {
    } else if (command == "breakpoint.set.on.function") {
    } else if (command == "watch.register.local.write") {
        if (operands.size() == 0) {
            cout << "error: invalid syntax, expected: <function-name> <register-index>..." << endl;
            verified = false;
        } else if (operands.size() == 1 and not str::isnum(operands[0])) {
            cout << "error: expected at least one integer operand" << endl;
            verified = false;
        }

        if (operands.size() > 0 and str::isnum(operands[0])) {
            // if operands start with numerical value, assume current function
            operands.insert(operands.begin(), ".");
        }
    } else if (command == "cpu.init") {
    } else if (command == "cpu.run") {
        if (not state.initialised) {
            cout << "error: CPU is not initialised, use `cpu.init` command before `" << command << "`" << endl;
            verified = false;
        } else if (state.paused) {
            cout << "warn: CPU is paused, use `cpu.resume` command instead of `" << command << "`" << endl;
            verified = false;
        }
    } else if (command == "cpu.resume") {
        if (not state.paused) {
            cout << "error: execution has not been paused, cannot resume" << endl;
            verified = false;
        } else if (operands.size() == 1) {
            if (not str::isnum(operands[0])) {
                cout << "error: invalid operand, expected integer" << endl;
                verified = false;
            }
        }
        if (operands.size() == 0) {
            operands.push_back("0");
        }
    } else if (command == "cpu.tick") {
        if (not state.initialised) {
            cout << "error: CPU is not initialised, use `cpu.init` command before `" << command << "`" << endl;
            verified = false;
        } else if (state.finished) {
            cout << "error: CPU has finished execution of loaded program" << endl;
            verified = false;
        } else if (state.paused) {
            cout << "warn: CPU is paused, use `cpu.resume` command instead of `" << command << "`" << endl;
            verified = false;
        } else if (operands.size() > 1) {
            cout << "error: invalid operand size, expected 0 or 1 operand but got " << operands.size() << endl;
            verified = false;
        } else if (operands.size() == 1 and not str::isnum(operands[0])) {
            cout << "error: invalid operand, expected integer" << endl;
            verified = false;
        }
    } else if (command == "cpu.jump") {
        if (operands.size() == 1) {
            if (not (str::isnum(operands[0]) or str::startswith(operands[0], "0x") or operands[0][0] == '+')) {
                cout << "error: invalid operand, expected:" << endl;
                cout << "  * decimal integer (optionally preceded by plus), or" << endl;
                cout << "  * hexadecimal integer" << endl;
                verified = false;
            }
        } else if (operands.size() == 2) {
            if (operands[0] == "sizeof" and not OP_SIZES.count(operands[1])) {
                cout << "error: invalid second operand, expected lowercase opcode name" << endl;
                verified = false;
            } else if (operands[0] != "sizeof") {
                cout << "error: invalid first operand, expected 'sizeof' subcommand" << endl;
                verified = false;
            }
        } else {
            cout << "error: invalid operand size, expected 1 operand" << endl;
            verified = false;
        }
    } else if (command == "cpu.unpause") {
        if (state.finished) {
            cout << "error: CPU has finished execution, use `cpu.unfinish` instead" << endl;
            verified = false;
        } else if (not state.paused) {
            cout << "warning: CPU has not been paused" << endl;
            verified = false;
        }
    } else if (command == "cpu.unfinish") {
        if (not state.finished) {
            cout << "error: CPU has not finished execution yet" << endl;
            verified = false;
        }
    } else if (command == "cpu.counter") {
    } else if (command == "register.show") {
    } else if (command == "register.local.show") {
    } else if (command == "register.global.show") {
    } else if (command == "register.static.show") {
    } else if (command == "print.ahead") {
    } else if (command == "arguments.show") {
    } else if (command == "trace" or command == "stack.trace" or command == "stack.trace.show") {
        command = "stack.trace.show";
    } else if (command == "stack.frame.show") {
    } else if (command == "loader.function.map" or command == "loader.function.map.show") {
        command = "loader.function.map.show";
        if (operands.size() == 0) {
            for (pair<string, unsigned> mapping : cpu.function_addresses) {
                operands.push_back(mapping.first);
            }
        }
    } else if (command == "loader.block.map" or command == "loader.block.map.show") {
        command = "loader.block.map.show";
        if (operands.size() == 0) {
            for (pair<string, unsigned> mapping : cpu.block_addresses) {
                operands.push_back(mapping.first);
            }
        }
    } else if (command == "loader.extern.function.map" or command == "loader.extern.function.map.show") {
        command = "loader.extern.function.map.show";
        if (operands.size() == 0) {
            for (pair<string, ExternalFunction*> mapping : cpu.external_functions) {
                operands.push_back(mapping.first);
            }
        }
    } else if (command == "quit") {
    } else if (command == "help") {
    } else {
        cout << "unknown command: `" << command << "`" << endl;
        verified = false;
    }

    return verified;
}
bool command_dispatch(string& command, vector<string>& operands, CPU& cpu, State& state) {
    /** Command dispatching logic.
     *
     *  This function will modify state of the debugger according to received command and
     *  its operands.
     *  Basic verification of both command and operands is performed.
     *
     *  Returns true on success, false otherwise.
     *  If false is returned, current iteration of debuggers's REPL should be skipped.
     */
    if (not command_verify(command, operands, cpu, state)) { return false; }

    if (command == "") {
        // do nothing...
    } else if (command == "conf.set") {
        if (operands[0] == "cpu.debug") {
            cpu.debug = (operands[0] == "true");
        }
    } else if (command == "conf.get") {
    } else if (command == "conf.load") {
        cout << "FIXME/TODO" << endl;
    } else if (command == "conf.load.default") {
        cout << "FIXME/TODO" << endl;
    } else if (command == "conf.dump") {
        cout << "FIXME/TODO" << endl;
    } else if (command == "breakpoint.set.at") {
        for (unsigned j = 0; j < operands.size(); ++j) {
            if (str::isnum(operands[j])) {
                state.breakpoints_byte.push_back(cpu.bytecode+stoi(operands[j]));
            } else {
                cout << "warn: invalid operand, expected integer: " << operands[j] << endl;
            }
        }
    } else if (command == "breakpoint.set.on.opcode") {
        for (unsigned j = 0; j < operands.size(); ++j) {
            state.breakpoints_opcode.push_back(operands[j]);
        }
    } else if (command == "breakpoint.set.on.function") {
        for (unsigned j = 0; j < operands.size(); ++j) {
            state.breakpoints_function.push_back(operands[j]);
        }
    } else if (command == "watch.register.local.write") {
        string function_name = (operands[0] == "." ? cpu.trace().back()->function_name : operands[0]);
        if (not state.watch_register_local_write.count(function_name)) {
            state.watch_register_local_write[function_name] = vector<int>({});
        }
        for (unsigned i = 1; i < operands.size(); ++i) {
            state.watch_register_local_write[function_name].push_back(stoi(operands[i]));
        }
    } else if (command == "cpu.init") {
        cpu.iframe();
        cpu.begin();
        state.initialised = true;
    } else if (command == "cpu.run") {
        state.ticks_left = -1;
    } else if (command == "cpu.resume") {
        state.autoresumes = stoi(operands[0]);
        state.paused = false;
        if (state.ticks_left == 0) {
            cout << "info: resumed, but ticks counter has already reached 0" << endl;
        }
    } else if (command == "cpu.tick") {
        state.ticks_left = (operands.size() ? stoi(operands[0]) : 1);
    } else if (command == "cpu.jump") {
        if (operands[0] == "sizeof") {
            cpu.instruction_pointer += OP_SIZES.at(operands[1]);
        } else if (operands[0][0] == '+') {
            unsigned long j = stoul(str::sub(operands[0], 1));
            while (j > 0) {
                string instruction;
                unsigned size;
                tie(instruction, size) = disassembler::instruction(cpu.instruction_pointer);
                cpu.instruction_pointer += size;
                --j;
            }
        } else if (str::startswith(operands[0], "0x")) {
            cpu.instruction_pointer = (cpu.bytecode+stoul(operands[0], nullptr, 16));
        } else {
            cpu.instruction_pointer = (cpu.bytecode+stoul(operands[0]));
        }
    } else if (command == "cpu.unpause") {
        state.paused = false;
        state.ticks_left = 0;
    } else if (command == "cpu.unfinish") {
        state.finished = false;
    } else if (command == "cpu.counter") {
        cout << cpu.counter() << endl;
    } else if (command == "register.show") {
        printRegisters(operands, cpu.uregset);
    } else if (command == "register.local.show") {
        printRegisters(operands, cpu.trace().back()->regset);
    } else if (command == "register.global.show") {
        printRegisters(operands, cpu.regset);
    } else if (command == "register.static.show") {
        string fun_name = cpu.trace().back()->function_name;

        try {
            printRegisters(operands, cpu.static_registers.at(fun_name));
        } catch (const std::out_of_range& e) {
            // OK, now we know that our function does not have static registers
            cout << "error: current function does not have static registers allocated" << endl;
        }
    } else if (command == "arguments.show") {
        printRegisters(operands, cpu.trace().back()->args);
    } else if (command == "print.ahead") {
        printInstruction(cpu);
    } else if (command == "stack.trace.show") {
        vector<Frame*> stack = cpu.trace();
        string indent("");
        for (unsigned j = 0; j < stack.size(); ++j) {
            cout << indent;
            cout << stringifyFunctionInvocation(stack[j]);
            cout << ": frame at " << hex << stack[j] << dec << endl;
            indent += " ";
        }
    } else if (command == "stack.frame.show") {
        Frame* top = cpu.trace().back();
        cout << "frame: " << stringifyFunctionInvocation(top) << '\n';
        cout << "  * index on stack: " << cpu.trace().size() << endl;
        cout << "  * return address:  " << top->ret_address() << endl;
        cout << "  * return value:    " << top->place_return_value_in << endl;
        cout << "  * resolve return:  " << (top->resolve_return_value_register ? "yes" : "no") << endl;
    } else if (command == "loader.function.map.show") {
        unsigned addr;
        bool exists = false;
        for (string fun : operands) {
            try {
                addr = cpu.function_addresses.at(fun);
                exists = true;
            } catch (const std::out_of_range& e) {
                exists = false;
            }
            cout << "  '" << fun << "': ";
            if (not exists) {
                cout << "not found" << endl;
            } else {
                cout << "entry point at byte " << addr << endl;
            }
        }
    } else if (command == "loader.block.map.show") {
        unsigned addr;
        bool exists = false;
        for (string fun : operands) {
            try {
                addr = cpu.block_addresses.at(fun);
                exists = true;
            } catch (const std::out_of_range& e) {
                exists = false;
            }
            cout << "  '" << fun << "': ";
            if (not exists) {
                cout << "not found" << endl;
            } else {
                cout << "entry point at byte " << addr << endl;
            }
        }
    } else if (command == "loader.function.map.show") {
        bool exists = false;
        for (string fun : operands) {
            try {
                cpu.external_functions.at(fun);
                exists = true;
            } catch (const std::out_of_range& e) {
                exists = false;
            }
            cout << "  '" << fun << "'";
            if (not exists) {
                cout << " (not found)" << endl;
            }
        }
    } else if (command == "help") {
        for (string c : DEBUGGER_COMMANDS) {
            if (c[c.size()-1] == '.') { continue; }
            cout << c << endl;
        }
    } else if (command == "quit") {
        state.quit = true;
    }

    // everything OK
    return true;
}


void completion(const char* buf, linenoiseCompletions* lc) {
    /** Completion callback for linenoise.
     *
     *  This function is a proxy between linenoise completion mechanism and
     *  dynamically built vector of all available completions.
     */
    unsigned len = strlen(buf);
    for (unsigned i = 0; i < DEBUGGER_COMMANDS.size(); ++i) {
        bool matching = true;
        unsigned j = 0;
        while (j < len and matching) {
            if (DEBUGGER_COMMANDS[i].size() < j) {
                matching = false;
                break;
            }
            matching = (DEBUGGER_COMMANDS[i][j] == buf[j]);
            ++j;
        }
        if (matching) {
            linenoiseAddCompletion(lc, DEBUGGER_COMMANDS[i].c_str());
        }
    }
}

void debuggerMainLoop(CPU& cpu, deque<string> init) {
    /** This function implements main REPL of the debugger.
     *
     *  Viua debugger is kind of interactive beast.
     *  You can enter commands into it and get results from them.
     *  The shell is very primitive (no scripting) and only accepts a few commands.
     */
    State state;

    deque<string> command_feed = init;
    string line(""), partline(""), lastline("");
    string command("");
    vector<string> operands;
    vector<string> chunks;


    /* Set the completion callback. This will be called every time the
     * user uses the <tab> key. */
    char word_separators[] = {'.'};
    linenoiseSetCompletionCallback(completion);
    linenoiseSetWordSeparators(1, word_separators);

    string home = getenv("HOME");
    string history = (home + DEBUGGER_COMMAND_HISTORY);
    linenoiseHistoryLoad(history.c_str());

    char* cline;
    while ((cline = linenoise(">>> ")) != NULL) {
        line = string(cline);
        linenoiseHistoryAdd(cline);
        free(cline);

        if (line == "") { continue; }
        if (line[0] == '#') { continue; }

        if (line == ".") {
            line = lastline;
        }
        if (line != "") {
            lastline = line;
        }

        command = str::chunk(line);
        operands = str::chunks(str::sub(line, command.size()));

        if (not command_dispatch(command, operands, cpu, state)) {
            continue;
        }

        if (state.quit) { break; }
        if (not state.initialised) { continue; }

        string op_name;
        while (not state.paused and not state.finished and not state.exception_raised and (state.ticks_left == -1 or (state.ticks_left > 0))) {
            if (state.ticks_left > 0) { --state.ticks_left; }

            OPCODE printed = NOP;
            try {
                printed = printInstruction(cpu);
            } catch (const std::out_of_range& e) {
                state.exception_type = "RuntimeException";
                state.exception_message = "unrecognised instruction";
                state.exception_raised = true;
            }

            byte* ticked = cpu.tick();
            if (not state.exception_raised and not state.finished and ticked == 0) {
                state.finished = (cpu.return_exception == "" ? true : false);
                state.ticks_left = 0;
                cout << "\nmessage: execution " << (cpu.return_exception == "" ? "finished" : "broken") << ": " << cpu.counter() << " instructions executed" << endl;
            }

            if (state.finished) {
                break;
            }
            if (printed == ECHO) {
                cout << endl;
            }

            if (cpu.return_exception != "") {
                state.exception_type = cpu.return_exception;
                state.exception_message = cpu.return_message;
                state.exception_raised = true;
            }

            if (state.exception_raised) {
                cout << state.exception_type << ": " << state.exception_message << endl;
                break;
            }

            string pause_reason = "";

            tie(state.paused, pause_reason) = if_breakpoint_byte(cpu, state.breakpoints_byte);
            if (not state.paused) {
                tie(state.paused, pause_reason) = if_breakpoint_opcode(cpu, state.breakpoints_opcode);
            }
            if (not state.paused) {
                tie(state.paused, pause_reason) = if_breakpoint_function(cpu, state.breakpoints_function);
            }
            if (not state.paused) {
                tie(state.paused, pause_reason) = if_watchpoint_local_register_write(cpu, state);
            }

            if (state.paused) {
                cout << pause_reason << endl;
            }


            try {
                op_name = OP_NAMES.at(OPCODE(*cpu.instruction_pointer));
            } catch (const std::out_of_range& e) {
                cout << "fatal: unknown instruction at byte " << (cpu.instruction_pointer-cpu.bytecode) << endl;
                state.autoresumes = 0;
                state.ticks_left = 0;
                state.paused = true;
                break;
            }

            if (state.paused) {
                // if there are no autoresumes, keep paused status
                // otherwise, set paused status to false
                state.paused = (not (state.autoresumes-- > 0));
            }
        }

        // do not let autoresumes slide to next execution
        state.autoresumes = 0;
    }

    linenoiseHistorySave(history.c_str());
}


bool usage(const char* program, bool SHOW_HELP, bool SHOW_VERSION, bool VERBOSE) {
    if (SHOW_HELP or (SHOW_VERSION and VERBOSE)) {
        cout << "Viua VM debugger, version ";
    }
    if (SHOW_HELP or SHOW_VERSION) {
        cout << VERSION << endl;
    }
    if (SHOW_HELP) {
        cout << "\nUSAGE:\n";
        cout << "    " << program << " [option...] <executable> [<operands>...]\n" << endl;
        cout << "OPTIONS:\n";
        cout << "    " << "-V, --version            - show version\n"
             << "    " << "-h, --help               - display this message\n"
             << "    " << "-v, --verbose            - show verbose output\n"
             ;
        cout << "\n";
        cout << "When inside debugger, use 'help' to list debugger statements (you can also discover them by pressing Tab repeatedly).";
        cout << "\n";
        cout << "Use ^C to abort entering a statement. Use ^D or 'quit' to quit the debugger.";
        cout << endl;
    }

    return (SHOW_HELP or SHOW_VERSION);
}

int main(int argc, char* argv[]) {
    // setup command line arguments vector
    vector<string> args;
    string option;
    for (int i = 1; i < argc; ++i) {
        option = string(argv[i]);
        if (option == "--help") {
            SHOW_HELP = true;
            continue;
        } else if (option == "--version") {
            SHOW_VERSION = true;
            continue;
        } else if (option == "--verbose") {
            VERBOSE = true;
        }
        args.push_back(argv[i]);
    }

    if (usage(argv[0], SHOW_HELP, SHOW_VERSION, VERBOSE)) { return 0; }

    if (args.size() == 0) {
        cout << "fatal: no input file" << endl;
        return 1;
    }

    string filename = "";
    filename = args[0];

    if (!filename.size()) {
        cout << "fatal: no file to run" << endl;
        return 1;
    }

    cout << "message: running \"" << filename << "\"" << endl;

    Loader loader(filename);
    loader.executable();

    uint16_t bytes = loader.getBytecodeSize();
    byte* bytecode = loader.getBytecode();

    cout << "bytecode size: " << bytes << endl;

    CPU cpu;
    cpu.debug = true;

    map<string, uint16_t> function_address_mapping = loader.getFunctionAddresses();
    uint16_t starting_instruction = function_address_mapping["__entry"];
    for (auto p : function_address_mapping) { cpu.mapfunction(p.first, p.second); }
    for (auto p : loader.getBlockAddresses()) { cpu.mapblock(p.first, p.second); }

    vector<string> cmdline_args;
    for (int i = 1; i < argc; ++i) {
        cmdline_args.push_back(argv[i]);
    }

    cpu.commandline_arguments = cmdline_args;
    cpu.load(bytecode).bytes(bytes).eoffset(starting_instruction);

    string homedir(getenv("HOME"));
    ifstream local_rc_file(homedir + RC_FILENAME);
    ifstream global_rc_file("/etc/viuavm/dbrc");

    deque<string> init_commands;
    string line;
    if (global_rc_file) {
        while (getline(global_rc_file, line)) { init_commands.push_back(line); }
    }
    if (local_rc_file) {
        while (getline(local_rc_file, line)) { init_commands.push_back(line); }
    }

    debuggerMainLoop(cpu, init_commands);

    return 0;
}
