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

    ifstream in(filename, ios::in | ios::binary);

    if (!in) {
        cout << "fatal: file could not be opened: " << filename << endl;
        return 1;
    }

    uint16_t function_ids_section_size = 0;
    char buffer[16];
    in.read(buffer, sizeof(uint16_t));
    function_ids_section_size = *((uint16_t*)buffer);

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

    uint16_t starting_instruction = function_address_mapping["__entry"];

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
    for (auto p : function_address_mapping) { cpu.mapfunction(p.first, p.second); }

    vector<string> cmdline_args;
    for (int i = 1; i < argc; ++i) {
        cmdline_args.push_back(argv[i]);
    }

    cpu.commandline_arguments = cmdline_args;

    cpu.load(bytecode).bytes(bytes).eoffset(starting_instruction).run();
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
