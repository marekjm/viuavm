#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include "../version.h"
#include "../bytecode/maps.h"
#include "../support/string.h"
#include "../support/pointer.h"
#include "../cpu/debugger.h"
#include "../program.h"
using namespace std;


const char* NOTE_LOADED_ASM = "note: seems like you have loaded an .asm file which cannot be run on CPU without prior compilation";
const char* RC_FILENAME = "/.wudoo.db.rc";


// MISC FLAGS
bool SHOW_HELP = false;
bool SHOW_VERSION = false;


string join(const string& s, const vector<string>& parts) {
    ostringstream oss;
    unsigned limit = parts.size();
    for (unsigned i = 0; i < limit; ++i) {
        oss << parts[i];
        if (i < (limit-1)) {
            oss << s;
        }
    }
    return oss.str();
}


byte* printIntegerOperand(byte* iptr) {
    cout << ((*(bool*)iptr) ? "@" : "");

    pointer::inc<bool, byte>(iptr);
    cout << *(int*)iptr;
    pointer::inc<int, byte>(iptr);
    return iptr;
}

void print1IntegerOperandInstruction(byte* iptr, const CPU& cpu) {
    printIntegerOperand(iptr);
}

void print2IntegerOperandInstruction(byte* iptr, const CPU& cpu) {
    iptr = printIntegerOperand(iptr);
    cout << ' ';
    iptr = printIntegerOperand(iptr);
}

void print3IntegerOperandInstruction(byte* iptr, const CPU& cpu) {
    iptr = printIntegerOperand(iptr);
    cout << ' ';
    iptr = printIntegerOperand(iptr);
    cout << ' ';
    iptr = printIntegerOperand(iptr);
}

void printRESS(byte* iptr, const CPU& cpu) {
    int to_register_set = *(int*)iptr;
    switch (to_register_set) {
        case 0:
            cout << "global";
            break;
        case 1:
            cout << "local";
            break;
        case 2:
            cout << "static (WIP)";
            break;
        case 3:
            cout << "temp (TODO)";
            break;
        default:
            cout << "illegal";
    }
}

void printJUMP(byte* iptr, const CPU& cpu) {
    cout << *(int*)iptr;
    pointer::inc<int, byte>(iptr);
}

void printBRANCH(byte* iptr, const CPU& cpu) {
    iptr = printIntegerOperand(iptr);
    cout << ' ';
    cout << *(int*)iptr;
    pointer::inc<int, byte>(iptr);
    cout << ' ';
    cout << *(int*)iptr;
    pointer::inc<int, byte>(iptr);
}

void printCALL(byte* iptr, const CPU& cpu) {
    string call_name = string(iptr);
    iptr += call_name.size();

    bool is_ref = *(bool*)iptr;
    pointer::inc<bool, byte>(iptr);
    bool return_value_register_num = *(int*)iptr;
    pointer::inc<int, byte>(iptr);

    cout << call_name << "; return address: " << (iptr-cpu.bytecode);
    if (return_value_register_num == 0) {
        cout << " (return value will be discarded)";
    } else {
        cout << " (return value will be placed in: ";
        cout << (is_ref ? "@" : "") << return_value_register_num << ')';
    }
}

typedef void(*PRINTER)(byte*, const CPU&);
map<string, PRINTER> opcode_printers = {
    { "izero", &print1IntegerOperandInstruction },
    { "istore", &print2IntegerOperandInstruction },
    { "iadd", &print3IntegerOperandInstruction },
    { "isub", &print3IntegerOperandInstruction },
    { "imul", &print3IntegerOperandInstruction },
    { "idiv", &print3IntegerOperandInstruction },
    { "iinc", &print1IntegerOperandInstruction },
    { "idec", &print1IntegerOperandInstruction },
    { "ilt", &print3IntegerOperandInstruction },
    { "ilte", &print3IntegerOperandInstruction },
    { "igt", &print3IntegerOperandInstruction },
    { "igte", &print3IntegerOperandInstruction },
    { "ieq", &print3IntegerOperandInstruction },

    { "itof", &print2IntegerOperandInstruction },
    { "ftoi", &print2IntegerOperandInstruction },

    { "echo", &print1IntegerOperandInstruction },
    { "print", &print1IntegerOperandInstruction },

    { "frame", &print2IntegerOperandInstruction },
    { "param", &print2IntegerOperandInstruction },
    { "paref", &print2IntegerOperandInstruction },
    { "arg", &print2IntegerOperandInstruction },

    { "move", &print2IntegerOperandInstruction },
    { "copy", &print2IntegerOperandInstruction },
    { "ref", &print2IntegerOperandInstruction },
    { "swap", &print2IntegerOperandInstruction },
    { "isnull", &print2IntegerOperandInstruction },
    { "ress", &printRESS },
    { "empty", &print1IntegerOperandInstruction },
    { "free", &print1IntegerOperandInstruction },

    { "tmpri", &print1IntegerOperandInstruction },
    { "tmpro", &print1IntegerOperandInstruction },

    { "and", &print3IntegerOperandInstruction },
    { "or", &print3IntegerOperandInstruction },
    { "not", &print1IntegerOperandInstruction },

    { "jump", &printJUMP },
    { "branch", &printBRANCH },
    { "call", &printCALL },
};


void printInstruction(const CPU& cpu) {
    byte* iptr = cpu.instruction_pointer;
    string opcode = OP_NAMES.at(OPCODE(*iptr));
    cout << "bytecode " << (iptr-cpu.bytecode) << " at 0x" << hex << long(iptr) << dec << ": " << opcode << ' ';

    ++iptr;
    try {
        (*opcode_printers.at(opcode))(iptr, cpu);
    } catch (const std::out_of_range& e) {
        cout << "<this opcode does not have a printer function>";
    }

    cout << endl;
}

void printRegisters(const vector<string>& indexes, int register_set_size, Object** registers, bool* references) {
    /** Print selected registers.
     *
     *  This function handles pretty printing of register contents.
     *  While printed, indexes are bound-checked.
     */
    int index;
    for (unsigned j = 0; j < indexes.size(); ++j) {
        if (not str::isnum(indexes[j])) {
            cout << "error: invalid operand, expected integer" << endl;
            continue;
        }
        index = stoi(indexes[j]);
        if (index >= register_set_size) {
            cout << "warn: register out of range: " << index << endl;
            continue;
        }
        cout << "-- " << index << " --";
        if (registers[index]) {
            cout << '\n';
            cout << "  reference:   " << (references[index] ? "true" : "false") << '\n';
            cout << "  object type: " << registers[index]->type() << '\n';
            cout << "  value:       " << registers[index]->repr() << '\n';
            cout << "  pointer:     " << hex << registers[index] << dec << endl;
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
    // ...

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
    } else if (command == "breakpoint.set.on" or command == "breakpoint.set.on.opcode") {
        command = "breakpoint.set.on.opcode";
    } else if (command == "breakpoint.set.on.function") {
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
        if (operands.size() != 1) {
            cout << "error: invalid operand size, expected 1 operand" << endl;
            verified = false;
        } else if (not str::isnum(operands[0])) {
            cout << "error: invalid operand, expected integer" << endl;
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
        if (not operands.size()) {
            cout << "error: invalid operand size, expected at least 1 operand" << endl;
            verified = false;
        }
    } else if (command == "register.global.show") {
        if (not operands.size()) {
            cout << "error: invalid operand size, expected at least 1 operand" << endl;
            verified = false;
        }
    } else if (command == "register.static.show") {
        if (not operands.size()) {
            cout << "error: invalid operand size, expected at least 1 operand" << endl;
            verified = false;
        }
    } else if (command == "trace" or command == "trace.show") {
        command = "trace.show";
    } else if (command == "loader.function.map" or command == "loader.function.map.show") {
        command = "loader.function.map.show";
        if (operands.size() == 0) {
            for (pair<string, unsigned> mapping : cpu.function_addresses) {
                operands.push_back(mapping.first);
            }
        }
    } else if (command == "quit") {
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
        cpu.instruction_pointer = (cpu.bytecode+stoi(operands[0]));
    } else if (command == "cpu.unpause") {
        state.paused = false;
        state.ticks_left = 0;
    } else if (command == "cpu.unfinish") {
        state.finished = false;
    } else if (command == "cpu.counter") {
        cout << cpu.counter() << endl;
    } else if (command == "register.show") {
        printRegisters(operands, cpu.uregisters_size, cpu.uregisters, cpu.ureferences);
    } else if (command == "register.global.show") {
        printRegisters(operands, cpu.reg_count, cpu.registers, cpu.references);
    } else if (command == "register.static.show") {
        string fun_name = cpu.trace().back()->function_name;

        Object** registers = 0;
        bool* references;
        int registers_size;
        try {
            tie(registers, references, registers_size) = cpu.static_registers.at(fun_name);
            printRegisters(operands, registers_size, registers, references);
        } catch (const std::out_of_range& e) {
            // OK, now we know that our function does not have static registers
            cout << "error: current function does not have static registers allocated" << endl;
        }
    } else if (command == "trace.show") {
        vector<Frame*> stack = cpu.trace();
        string indent("");
        for (unsigned j = 1; j < stack.size(); ++j) {
            cout << indent << stack[j]->function_name << endl;
            indent += " ";
        }
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
                cout << addr << endl;
            }
        }
    } else if (command == "quit") {
        state.quit = true;
    }


    // everything OK
    return true;
}

void debuggerMainLoop(CPU& cpu, deque<string> init) {
    /** This function implements main REPL of the debugger.
     *
     *  Wudoo debugger is kind of interactive beast.
     *  You can enter commands into it and get results from them.
     *  The shell is very primitive (no scripting) and only accepts a few commands.
     */
    State state;

    deque<string> command_feed = init;
    string line(""), partline(""), lastline("");
    string command("");
    vector<string> operands;
    vector<string> chunks;

    while (true) {
        if (not command_feed.size()) {
            cout << ">>> ";
            getline(cin, line);
            line = str::lstrip(line);

            // handle ^D
            if (cin.eof()) {
                cin.clear(); // EOF must be cleared
                cout << "^D" << endl;
                continue;
            }
        }

        if (line == string("{")) {
            while (true) {
                cout << "...  ";
                getline(cin, partline);
                if (partline == string("}")) {
                    break;
                }
                command_feed.push_back(partline);
            }
            partline = "";
        }

        if (command_feed.size()) {
            line = command_feed.front();
            command_feed.pop_front();
        }

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

            try {
                printInstruction(cpu);
            } catch (const std::out_of_range& e) {
                state.exception_type = "RuntimeException";
                state.exception_message = "unrecognised instruction";
                state.exception_raised = true;
            }

            if (not state.exception_raised and not state.finished and cpu.tick() == 0) {
                state.finished = (cpu.return_exception == "" ? true : false);
                state.ticks_left = 0;
                cout << "\nmessage: execution " << (cpu.return_exception == "" ? "finished" : "broken") << ": " << cpu.counter() << " instructions executed" << endl;
            }

            if (state.finished) {
                break;
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
        }
        args.push_back(argv[i]);
    }

    if (SHOW_HELP or SHOW_VERSION) {
        cout << "wudoo VM virtual machine debugger, version " << VERSION << endl;
        if (SHOW_HELP) {
            cout << "    --help             - to display this message" << endl;
            cout << "    --version          - show version and quit" << endl;
            cout << "\n\nDebugger statements:\n";
            cout << "  cpu.init                     - initialise the CPU (must be run before starting CPU)\n"
                 << "  cpu.tick [n]                 - perform [n] CPU ticks, [n] is optional, [n] can be -1 to run to the end of code\n"
                 << "  cpu.resume                   - resume execution after breakpoint\n"
                 << "  cpu.unpause                  - unpause CPU after breakpoint (does not resume execution)\n"
                 << "  cpu.jump <byte>              - set instruction pointer to <byte>\n"
                 << "  breakpoint.set.at <byte>     - set breakpoint at <byte>\n"
                 << "  breakpoint.set.on <opcode>+  - set breakpoint on <opcode> (accepts space-separated multiple operands)\n"
                 << "  st.show                      - show stack trace\n"
                 << "  register.show <n>...         - show register n from current register set (accepts space-separated multiple operands)\n"
                 << "  register.local.show <n>...   - show local register n (accepts space-separated multiple operands)\n"
                 << "  register.static.show <n>...  - show static register n (accepts space-separated multiple operands)\n"
                 << "  register.global.show <n>...  - show global register n (accepts space-separated multiple operands)\n"
                 << "  .                            - reuse last statement (last statement in bulk, it does not support blocks)\n"
                 ;
            cout << "\nMultiple statements can be given at once and executed in bulk like this:\n\n";
            cout << "  >>> {\n"
                 << "  ...  cpu.init\n"
                 << "  ...  breakpoint.set.on jump end halt\n"
                 << "  ...  cpu.tick -1\n"
                 << "  ...  }\n"
                 ;
            cout << "\n";
            cout << "Use ^D to abort entering a statement. Use ^C to quit the debugger.";
            cout << endl;
        }
        return 0;
    }

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

    ifstream in(filename, ios::in | ios::binary);

    if (!in) {
        cout << "fatal: file could not be opened: " << filename << endl;
        return 1;
    }

    uint16_t function_ids_section_size = 0;
    char buffer[16];
    in.read(buffer, sizeof(uint16_t));
    function_ids_section_size = *((uint16_t*)buffer);
    cout << "message: function mapping section: " << function_ids_section_size << " bytes" << endl;

    /*  The code below extracts function id-to-address mapping.
     */
    map<string, uint16_t> function_address_mapping;
    char *buffer_function_ids = new char[function_ids_section_size];
    in.read(buffer_function_ids, function_ids_section_size);
    char *function_ids_map = buffer_function_ids;

    int i = 0;
    string fn_name;
    uint16_t fn_address;
    while (i < function_ids_section_size) {
        fn_name = string(function_ids_map);
        i += fn_name.size() + 1;  // one for null character
        fn_address = *((uint16_t*)(buffer_function_ids+i));
        i += sizeof(uint16_t);
        function_ids_map = buffer_function_ids+i;
        function_address_mapping[fn_name] = fn_address;

        cout << "debug: function id-to-address mapping: " << fn_name << " @ byte " << fn_address << endl;
    }
    delete[] buffer_function_ids;


    uint16_t bytes;

    in.read(buffer, 16);
    if (!in) {
        cout << "fatal: an error occued during bytecode loading: cannot read size" << endl;
        if (str::endswith(filename, ".asm")) { cout << NOTE_LOADED_ASM << endl; }
        return 1;
    } else {
        bytes = *((uint16_t*)buffer);
    }
    cout << "message: bytecode size: " << bytes << " bytes" << endl;

    uint16_t starting_instruction = function_address_mapping["__entry"];
    cout << "message: first executable instruction at byte " << starting_instruction << endl;

    byte* bytecode = new byte[bytes];
    in.read((char*)bytecode, bytes);

    if (!in) {
        cout << "fatal: an error occued during bytecode loading: cannot read instructions" << endl;
        if (str::endswith(filename, ".asm")) { cout << NOTE_LOADED_ASM << endl; }
        return 1;
    }
    in.close();

    int ret_code = 0;
    string return_exception = "", return_message = "";
    // run the bytecode
    CPU cpu;
    cpu.debug = true;
    for (auto p : function_address_mapping) { cpu.mapfunction(p.first, p.second); }

    vector<string> cmdline_args;
    for (int i = 1; i < argc; ++i) {
        cmdline_args.push_back(argv[i]);
    }

    cpu.commandline_arguments = cmdline_args;
    cpu.load(bytecode).bytes(bytes).eoffset(starting_instruction);

    string homedir(getenv("HOME"));
    ifstream local_rc_file(homedir + RC_FILENAME);
    ifstream global_rc_file("/etc/wudoovm/dbrc");

    deque<string> init_commands;
    string line;
    if (global_rc_file) {
        while (getline(global_rc_file, line)) { init_commands.push_back(line); }
    }
    if (local_rc_file) {
        while (getline(local_rc_file, line)) { init_commands.push_back(line); }
    }

    debuggerMainLoop(cpu, init_commands);

    return ret_code;
}
