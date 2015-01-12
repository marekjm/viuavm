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

        uint16_t bytes;
        uint16_t starting_instruction;
        char buffer[16];

        in.read(buffer, 16);
        if (!in) {
            cout << "fatal: an error occued during bytecode loading: cannot read size" << endl;
            if (str::endswith(filename, ".asm")) { cout << NOTE_LOADED_ASM << endl; }
            return 1;
        } else {
            bytes = *((uint16_t*)buffer);
        }

        in.read(buffer, 16);
        if (!in) {
            cout << "fatal: an error occued during bytecode loading: cannot read executable offset" << endl;
            if (str::endswith(filename, ".asm")) { cout << NOTE_LOADED_ASM << endl; }
            return 1;
        } else {
            starting_instruction = *((uint16_t*)buffer);
        }

        byte* bytecode = new byte[bytes];
        in.read((char*)bytecode, bytes);

        if (!in) {
            cout << "fatal: an error occued during bytecode loading: cannot read instructions" << endl;
            if (str::endswith(filename, ".asm")) { cout << NOTE_LOADED_ASM << endl; }
            return 1;
        } else {
            starting_instruction = *((uint16_t*)buffer);
        }
        in.close();

        // run the bytecode
        CPU cpu;
        cpu.debug = debug;
        ret_code = cpu.load(bytecode).bytes(bytes).eoffset(starting_instruction).run();
    } else {
        cout << "tatanka VM, version " << VERSION << endl;
        if (argc > 1 and args[1] == "--help") {
            cout << args[0] << " [--debug] <infile>         - to run a program" << endl;
            cout << args[0] << " [--help]                   - to display this message" << endl;
        }
    }

    return ret_code;
}
