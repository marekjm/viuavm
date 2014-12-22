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
    // print hello message
    cout << "tatanka VM, version " << VERSION << endl;


    // setup command line arguments vector
    vector<string> args;
    for (int i = 0; i < argc; ++i) { args.push_back(argv[i]); }


    int ret_code = 0;


    // run code
    if (argc > 1) {
        cout << args[1] << endl;

        int runmode = BINARY;
        if (args[1] == "--bin") {
            cout << "debug: setting runmode to 'binary'" << endl;
            runmode = BINARY;
        } else if (args[1] == "--asm") {
            cout << "debug: setting runmode to 'asm'" << endl;
            runmode = ASM;
        }

        string filename = "";
        if (argc > 2) {
            filename = args[2];
        }

        cout << "message: running in " << (runmode == ASM ? "asm" : "binary") << " mode" << endl;

        if (!filename.size()) {
            cout << "fatal: no file to run" << endl;
            return 1;
        }

        ifstream in(filename, ios::in | ios::binary);

        if (!in) {
            cout << "fatal: file could not be opened" << endl;
            return 1;
        } else {
            cout << "debug: reading file: " << argv[1] << endl;
        }

        uint16_t bytes;
        char buffer[16];
        in.read(buffer, 16);

        if (!in) {
            cout << "fatal: an error occued during loading of the program" << endl;
            return 1;
        }

        bytes = *((uint16_t*)buffer);

        cout << "debug: program is " << bytes << " bytes" << endl;

        byte* bytecode = new byte[bytes];
        in.read(bytecode, bytes);
        in.close();

        cout << "debug: running file: " << filename << endl;

        CPU cpu(64);
        cpu.load(bytecode).bytes(bytes);
        ret_code = cpu.run();
    }

    return ret_code;
}
