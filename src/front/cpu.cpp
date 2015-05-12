#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "../version.h"
#include "../support/string.h"
#include "../loader.h"
#include "../cpu/cpu.h"
#include "../program.h"
using namespace std;


const char* NOTE_LOADED_ASM = "note: seems like you have loaded an .asm file which cannot be run on CPU without prior compilation";


// MISC FLAGS
bool SHOW_HELP = false;
bool SHOW_VERSION = false;


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
        cout << "wudoo VM virtual machine, version " << VERSION << endl;
        if (SHOW_HELP) {
            cout << "    --help             - display this message" << endl;
            cout << "    --version          - display version" << endl;
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

    Loader loader(filename);
    loader.executable();

    map<string, uint16_t> function_address_mapping = loader.getFunctionAddresses();
    uint16_t starting_instruction = function_address_mapping["__entry"];

    uint16_t bytes = loader.getBytecodeSize();
    byte* bytecode = loader.getBytecode();

    CPU cpu;
    for (auto p : function_address_mapping) { cpu.mapfunction(p.first, p.second); }
    for (auto p : loader.getBlockAddresses()) { cpu.mapblock(p.first, p.second); }

    vector<string> cmdline_args;
    for (int i = 1; i < argc; ++i) {
        cmdline_args.push_back(argv[i]);
    }

    cpu.commandline_arguments = cmdline_args;

    cpu.load(bytecode).bytes(bytes).eoffset(starting_instruction).run();

    int ret_code = 0;
    string return_exception = "", return_message = "";
    tie(ret_code, return_exception, return_message) = cpu.exitcondition();

    if (ret_code != 0 and return_exception.size()) {
        vector<Frame*> trace = cpu.trace();
        cout << "stack trace: from entry point...\n";
        for (unsigned i = 1; i < trace.size(); ++i) {
            cout << "  called function: '" << trace[i]->function_name << "'\n";
        }
        cout << "exception after " << cpu.counter() << " ticks: ";
        cout << return_exception << ": " << return_message << endl;

        cout << "frame details:\n";

        Frame* last = trace.back();
        if (last->regset->size()) {
            unsigned non_empty = 0;
            for (unsigned r = 0; r < last->regset->size(); ++r) {
                if (last->regset->at(r) != 0) { ++non_empty; }
            }
            cout << "  non-empty registers: " << non_empty << '/' << last->regset->size();
            cout << (non_empty ? ":\n" : "\n");
            for (unsigned r = 0; r < last->regset->size(); ++r) {
                if (last->regset->at(r) == 0) { continue; }
                cout << "    registers[" << r << "]: ";
                cout << '<' << last->regset->get(r)->type() << "> " << last->regset->get(r)->str() << endl;
            }
        } else {
            cout << "  no registers were allocated for this frame" << endl;
        }

        if (last->args->size()) {
            cout << "    non-empty arguments (out of " << last->args->size() << "):" << endl;
            for (unsigned r = 0; r < last->args->size(); ++r) {
                if (last->args->at(r) == 0) { continue; }
                cout << "    arguments[" << r << "]: ";
                cout << '<' << last->args->get(r)->type() << "> " << last->args->get(r)->str() << endl;
            }
        } else {
            cout << "  no arguments were passed to this frame" << endl;
        }
    }

    return ret_code;
}
