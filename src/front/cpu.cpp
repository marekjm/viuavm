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
bool ANALYZE = false;


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
        } else if (option == "--analyze") {
            ANALYZE = true;
            continue;
        }
        args.push_back(argv[i]);
    }

    int ret_code = 0;

    if (SHOW_HELP or SHOW_VERSION) {
        cout << "wudoo VM virtual machine, version " << VERSION << endl;
        if (SHOW_HELP) {
            cout << "    --debug <infile>       - to run a program in debug mode (shows debug output)" << endl;
            cout << "    --stepping <infile>    - to run a program in stepping mode (pauses after each instruction, implies debug mode)" << endl;
            cout << "    --help                 - to display this message" << endl;
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
    uint16_t starting_instruction;

    in.read(buffer, 16);
    if (!in) {
        cout << "fatal: an error occued during bytecode loading: cannot read size" << endl;
        if (str::endswith(filename, ".asm")) { cout << NOTE_LOADED_ASM << endl; }
        return 1;
    } else {
        bytes = *((uint16_t*)buffer);
    }
    if (VERBOSE or DEBUG) { cout << "message: bytecode size: " << bytes << " bytes" << endl; }

    in.read(buffer, 16);
    if (!in) {
        cout << "fatal: an error occued during bytecode loading: cannot read executable offset" << endl;
        if (str::endswith(filename, ".asm")) { cout << NOTE_LOADED_ASM << endl; }
        return 1;
    } else {
        starting_instruction = *((uint16_t*)buffer);
    }
    if (VERBOSE or DEBUG) { cout << "message: first executable instruction at byte " << starting_instruction << endl; }

    byte* bytecode = new byte[bytes];
    in.read((char*)bytecode, bytes);

    if (!in) {
        cout << "fatal: an error occued during bytecode loading: cannot read instructions" << endl;
        if (str::endswith(filename, ".asm")) { cout << NOTE_LOADED_ASM << endl; }
        return 1;
    }
    in.close();

    // run the bytecode
    CPU cpu;
    cpu.debug = (DEBUG or STEP_BY_STEP);
    cpu.stepping = STEP_BY_STEP;
    for (auto p : function_address_mapping) { cpu.mapfunction(p.first, p.second); }

    if (not ANALYZE) {
        ret_code = cpu.load(bytecode).bytes(bytes).eoffset(starting_instruction).run();
    }

    return ret_code;
}
