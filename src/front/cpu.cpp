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


enum RunMode {
    BINARY,
    ASM,
};


int main(int argc, char* argv[]) {
    // setup command line arguments vector
    vector<string> args;
    for (int i = 0; i < argc; ++i) { args.push_back(argv[i]); }

    int ret_code = 0;

    // run code
    if (argc > 1 and args[1] != "--help") {
        bool debug = false;
        string filename;
        if (args[1] == "--debug") {
            debug = true;
            if (argc >= 3) {
                filename = args[2];
            } else {
                cout << "fatal: no file to run" << endl;
                return 1;
            }
        } else {
            filename = args[1];
        }

        if (!filename.size()) {
            cout << "fatal: no file to run" << endl;
            return 1;
        }

        ifstream in(filename, ios::in | ios::binary);

        if (!in) {
            cout << "fatal: file could not be opened" << endl;
            return 1;
        }

        uint16_t function_ids_section_size = 0;
        char buffer[16];
        in.read(buffer, sizeof(uint16_t));
        function_ids_section_size = *((uint16_t*)buffer);
        if (debug) { cout << "debug: function mapping section: " << function_ids_section_size << " bytes" << endl; }

        char *buffer_function_ids = new char[function_ids_section_size];
        in.read(buffer_function_ids, function_ids_section_size);
        char *function_ids_map = buffer_function_ids;

        map<string, uint16_t> function_address_mapping;

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

            if (debug) {
                cout << "debug: function id-to-address mapping: " << fn_name << " @ byte " << fn_address << endl;
            }
        }

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
        if (debug) { cout << "debug: bytecode size: " << bytes << " bytes" << endl; }

        in.read(buffer, 16);
        if (!in) {
            cout << "fatal: an error occued during bytecode loading: cannot read executable offset" << endl;
            if (str::endswith(filename, ".asm")) { cout << NOTE_LOADED_ASM << endl; }
            return 1;
        } else {
            starting_instruction = *((uint16_t*)buffer);
        }
        if (debug) { cout << "debug: first executable instruction at byte " << starting_instruction << endl; }

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
        cpu.debug = debug;
        for (auto p : function_address_mapping) {
            if (debug) {
                cout << "debug: ";
                cout << "mapped function '" << p.first << "' to address " << p.second << " in CPU" << endl;
            }
            cpu.mapfunction(p.first, p.second);
        }

        ret_code = cpu.load(bytecode).bytes(bytes).eoffset(starting_instruction).run();
    } else {
        cout << "wudoo VM, version " << VERSION << endl;
        if (argc > 1 and args[1] == "--help") {
            cout << args[0] << " [--debug] <infile>         - to run a program" << endl;
            cout << args[0] << " [--help]                   - to display this message" << endl;
        }
    }

    return ret_code;
}
