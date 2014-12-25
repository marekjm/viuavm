#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "../version.h"
#include "../cpu.h"
#include "../bytecode.h"
#include "../program.h"
using namespace std;


typedef char byte;


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
    if (argc > 1) {
        bool debug = false;
        string filename;
        if (args[1] == "--debug") {
            debug = true;
            filename = args[2];
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
        char buffer[16];
        in.read(buffer, 16);

        if (!in) {
            cout << "fatal: an error occued during loading of the program" << endl;
            return 1;
        }

        bytes = *((uint16_t*)buffer);

        byte* bytecode = new byte[bytes];
        in.read(bytecode, bytes);
        in.close();

        // run the bytecode
        CPU cpu;
        cpu.debug = debug;
        ret_code = cpu.load(bytecode).bytes(bytes).run();
    } else {
        cout << "tatanka VM, version " << VERSION << endl;
        cout << args[0] << " [--debug] <infile>" << endl;
    }

    return ret_code;
}
