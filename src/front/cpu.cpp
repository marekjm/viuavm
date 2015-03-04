#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "../version.h"
#include "../support/string.h"
#include "../cpu/cpu.h"
#include "../program.h"
using namespace std;


const char* NOTE_LOADED_ASM = "note: seems like you have loaded an .asm file which cannot be run on CPU without prior compilation";


// MISC FLAGS
bool SHOW_HELP = false;
bool SHOW_VERSION = false;
bool VERBOSE = false;
bool DEBUG = false;
bool STEP_BY_STEP = false;


// WARNING FLAGS
bool WARNING_ALL = false;


// ERROR FLAGS
bool ERROR_ALL = false;


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
            continue;
        } else if (option == "--debug") {
            DEBUG = true;
            continue;
        } else if (option == "--Wall") {
            WARNING_ALL = true;
            continue;
        } else if (option == "--Eall") {
            ERROR_ALL = true;
            continue;
        } else if (option == "--step") {
            STEP_BY_STEP = true;
            continue;
        }
        args.push_back(argv[i]);
    }

    if (SHOW_HELP or SHOW_VERSION) {
        cout << "wudoo VM virtual machine, version " << VERSION << endl;
        if (SHOW_HELP) {
            cout << "    --analyze          - to display information about loaded bytecode but not run it" << endl;
            cout << "    --debug <infile>   - to run a program in debug mode (shows debug output)" << endl;
            cout << "    --help             - to display this message" << endl;
            cout << "    --step <infile>    - to run a program in stepping mode (pauses after each instruction, implies debug mode for CPU)" << endl;
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

    if (VERBOSE or DEBUG) {
        cout << "message: running \"" << filename << "\"" << endl;
    }

    ifstream in(filename, ios::in | ios::binary);

    if (!in) {
        cout << "fatal: file could not be opened: " << filename << endl;
        return 1;
    }

    uint16_t function_ids_section_size = 0;
    char buffer[16];
    in.read(buffer, sizeof(uint16_t));
    function_ids_section_size = *((uint16_t*)buffer);
    if (VERBOSE or DEBUG) { cout << "message: function mapping section: " << function_ids_section_size << " bytes" << endl; }

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

        if (DEBUG) {
            cout << "debug: function id-to-address mapping: " << fn_name << " @ byte " << fn_address << endl;
        }
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
    if (VERBOSE or DEBUG) { cout << "message: bytecode size: " << bytes << " bytes" << endl; }

    uint16_t starting_instruction = function_address_mapping["__entry"];
    if (VERBOSE or DEBUG) { cout << "message: first executable instruction at byte " << starting_instruction << endl; }

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
    cpu.debug = (DEBUG or STEP_BY_STEP);
    cpu.stepping = STEP_BY_STEP;
    for (auto p : function_address_mapping) { cpu.mapfunction(p.first, p.second); }

    cpu.load(bytecode).bytes(bytes).eoffset(starting_instruction).run();
    tie(ret_code, return_exception, return_message) = cpu.exitcondition();

    if (VERBOSE or DEBUG) {
        if (STEP_BY_STEP) {
            // we need extra newline to separate VM CPU output from CPU frontend output
            cout << '\n';
        }
        cout << "message: execution " << (return_exception.size() == 0 ? "finished" : "broken") << ": " << cpu.counter() << " instructions executed" << endl;
    }

    if (ret_code != 0 and return_exception.size()) {
        vector<Frame*> trace = cpu.trace();
        cout << "stack trace: from entry point...\n";
        for (unsigned i = 1; i < trace.size(); ++i) {
            cout << "  called function: '" << trace[i]->function_name << "'\n";
        }
        cout << "exception in function '" << trace.back()->function_name << "': ";
        cout << return_exception << ": " << return_message << endl;

        cout << "frame details:\n";

        Frame* last = trace.back();
        if (last->registers_size) {
            unsigned non_empty = 0;
            for (int r = 0; r < last->registers_size; ++r) {
                if (last->registers[r] != 0) { ++non_empty; }
            }
            cout << "  non-empty registers: " << non_empty << '/' << last->registers_size;
            cout << (non_empty ? ":\n" : "\n");
            for (int r = 0; r < last->registers_size; ++r) {
                if (last->registers[r] == 0) { continue; }
                cout << "    registers[" << r << "]: ";
                cout << '<' << last->registers[r]->type() << "> " << last->registers[r]->str() << endl;
            }
        } else {
            cout << "  no registers were allocated for this frame" << endl;
        }

        if (last->arguments_size) {
            cout << "    non-empty arguments (out of " << last->arguments_size << "):" << endl;
            for (int r = 0; r < last->arguments_size; ++r) {
                if (last->arguments[r] == 0) { continue; }
                cout << "    arguments[" << i << "]: ";
                cout << '<' << last->arguments[r]->type() << "> " << last->arguments[r]->str() << endl;
            }
        } else {
            cout << "  no arguments were passed to this frame" << endl;
        }
    }

    return ret_code;
}
