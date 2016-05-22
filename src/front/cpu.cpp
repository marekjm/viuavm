#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <viua/version.h>
#include <viua/bytecode/maps.h>
#include <viua/cg/disassembler/disassembler.h>
#include <viua/program.h>
#include <viua/printutils.h>
#include <viua/front/vm.h>
using namespace std;


const char* NOTE_LOADED_ASM = "note: seems like you have loaded an .asm file which cannot be run on CPU without prior compilation";


// MISC FLAGS
bool SHOW_HELP = false;
bool SHOW_VERSION = false;
bool VERBOSE = false;


bool usage(const char* program, bool SHOW_HELP, bool SHOW_VERSION, bool VERBOSE) {
    if (SHOW_HELP or (SHOW_VERSION and VERBOSE)) {
        cout << "Viua VM CPU, version ";
    }
    if (SHOW_HELP or SHOW_VERSION) {
        cout << VERSION << '.' << MICRO << ' ' << COMMIT << endl;
    }
    if (SHOW_HELP) {
        cout << "\nUSAGE:\n";
        cout << "    " << program << " [option...] <executable>\n" << endl;
        cout << "OPTIONS:\n";
        cout << "    " << "-V, --version            - show version\n"
             << "    " << "-h, --help               - display this message\n"
             << "    " << "-v, --verbose            - show verbose output\n"
             ;
    }

    return (SHOW_HELP or SHOW_VERSION);
}

int main(int argc, char* argv[]) {
    // setup command line arguments vector
    vector<string> args;
    string option;
    for (int i = 1; i < argc; ++i) {
        option = string(argv[i]);
        if (option == "--help" or option == "-h") {
            SHOW_HELP = true;
            continue;
        } else if (option == "--version" or option == "-V") {
            SHOW_VERSION = true;
            continue;
        } else if (option == "--verbose" or option == "-v") {
            VERBOSE = true;
            continue;
        } else if (str::startswith(option, "-")) {
            cout << "error: unknown option: " << option << endl;
            return 1;
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
    if (!support::env::isfile(filename)) {
        cout << "fatal: could not open file: " << filename << endl;
        return 1;
    }

    CPU cpu;
    viua::front::vm::initialise(&cpu, filename, args);

    try {
        // try preloading dynamic libraries specified by environment
        viua::front::vm::preload_libraries(&cpu);
    } catch (const Exception* e) {
        cout << "fatal: preload: " << e->what() << endl;
        return 1;
    }

    viua::front::vm::load_standard_prototypes(&cpu);

    try {
        cpu.run();
    } catch (const Exception* e) {
        cout << "fatal: " << e->what() << endl;
        return 1;
    }

    int ret_code = 0;
    string return_exception = "", return_message = "";
    tie(ret_code, return_exception, return_message) = cpu.exitcondition();

    if (cpu.terminated()) {
        vector<Frame*> trace = cpu.trace();
        cout << "stack trace: from entry point, most recent call last...\n";
        for (unsigned i = 1; i < trace.size(); ++i) {
            cout << "  " << stringifyFunctionInvocation(trace[i]) << "\n";
        }
        cout << "\n";

        Type* thrown_object = cpu.terminatedBy();
        Exception* ex = dynamic_cast<Exception*>(thrown_object);
        string ex_type = thrown_object->type();

        cout << "exception after " << cpu.counter() << " ticks" << endl;
        cout << "failed instruction: " << get<0>(disassembler::instruction(cpu.executionAt())) << endl;
        cout << "uncaught object: " << ex_type << " = " << (ex ? ex->what() : thrown_object->str()) << endl;
        cout << "\n";

        cout << "frame details:\n";

        if (trace.size()) {
            Frame* last = trace.back();
            if (last->regset->size()) {
                unsigned non_empty = 0;
                for (unsigned r = 0; r < last->regset->size(); ++r) {
                    if (last->regset->at(r) != nullptr) { ++non_empty; }
                }
                cout << "  non-empty registers: " << non_empty << '/' << last->regset->size();
                cout << (non_empty ? ":\n" : "\n");
                for (unsigned r = 0; r < last->regset->size(); ++r) {
                    if (last->regset->at(r) == nullptr) { continue; }
                    cout << "    registers[" << r << "]: ";
                    cout << '<' << last->regset->get(r)->type() << "> " << last->regset->get(r)->str() << endl;
                }
            } else {
                cout << "  no registers were allocated for this frame" << endl;
            }

            if (last->args->size()) {
                cout << "  non-empty arguments (out of " << last->args->size() << "):" << endl;
                for (unsigned r = 0; r < last->args->size(); ++r) {
                    if (last->args->at(r) == nullptr) { continue; }
                    cout << "    arguments[" << r << "]: ";
                    if (last->args->isflagged(r, MOVED)) {
                        cout << "[moved] ";
                    }
                    if (Pointer* ptr = dynamic_cast<Pointer*>(last->args->get(r))) {
                        if (ptr->expired()) {
                            cout << "<ExpiredPointer>" << endl;
                        } else {
                            cout << '<' << ptr->type() << '>' << endl;
                        }
                    } else {
                        cout << '<' << last->args->get(r)->type() << "> " << last->args->get(r)->str() << endl;
                    }
                }
            } else {
                cout << "  no arguments were passed to this frame" << endl;
            }
        } else {
            cout << "no stack trace available" << endl;
        }
    }

    return ret_code;
}
