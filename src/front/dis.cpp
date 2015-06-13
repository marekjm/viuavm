#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "../version.h"
#include "../bytecode/opcodes.h"
#include "../bytecode/maps.h"
#include "../support/string.h"
#include "../support/pointer.h"
#include "../loader.h"
using namespace std;


// MISC FLAGS
bool SHOW_HELP = false;
bool SHOW_VERSION = false;
bool VERBOSE = false;

bool DISASSEMBLE_ENTRY = false;
bool INCLUDE_INFO = false;


string printIntegerOperand(byte* iptr) {
    cout << ((*(bool*)iptr) ? "@" : "");

    pointer::inc<bool, byte>(iptr);
    cout << *(int*)iptr;
    pointer::inc<int, byte>(iptr);
    return iptr;
}

int main(int argc, char* argv[]) {
    // setup command line arguments vector
    vector<string> args;
    string option;
    for (int i = 1; i < argc; ++i) {
        option = string(argv[i]);
        if (option == "--help") {
            SHOW_HELP = true;
        } else if (option == "--version") {
            SHOW_VERSION = true;
        } else if (option == "--verbose") {
            VERBOSE = true;
        } else if ((option == "--with-entry") or (option == "-e")) {
            DISASSEMBLE_ENTRY = true;
        } else if ((option == "--info") or (option == "-i")) {
            INCLUDE_INFO = true;
        } else {
            args.push_back(argv[i]);
        }
    }

    if (SHOW_HELP or (SHOW_VERSION and VERBOSE)) {
        cout << "Viua VM disassembler, version ";
    }
    if (SHOW_HELP or SHOW_VERSION) {
        cout << VERSION << endl;
    }
    if (SHOW_HELP) {
        cout << "    --help             - to display this message" << endl;
        cout << "    --version          - show version and quit" << endl;
        cout << "    --verbose          - show verbose output" << endl;
        cout << "    --with-entry       - disassemble entry function" << endl;
        cout << "    --info             - include info about disassembled file in output" << endl;
        cout << endl;
    }
    if (SHOW_HELP or SHOW_VERSION) {
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

    uint16_t bytes = loader.getBytecodeSize();
    byte* bytecode = loader.getBytecode();

    map<string, uint16_t> function_address_mapping = loader.getFunctionAddresses();
    vector<string> functions = loader.getFunctions();

    map<string, uint16_t> block_address_mapping = loader.getBlockAddresses();
    vector<string> blocks = loader.getBlocks();

    vector<string> disassembled_lines;

    ostringstream oss;

    oss << "; bytecode size: " << bytes << '\n';
    oss << ";\n";
    oss << "; functions:\n";
    for (unsigned i = 0; i < functions.size(); ++i) {
        oss << ";   " << functions[i] << " at byte " << function_address_mapping[functions[i]] << '\n';
    }
    oss << "\n\n";

    disassembled_lines.push_back(oss.str());

    string name;
    unsigned fn_size;
    for (unsigned i = 0; i < functions.size(); ++i) {
        name = functions[i];
        if ((name == "__entry") and not DISASSEMBLE_ENTRY) {
            continue;
        }
        oss.str("");

        if (i < (functions.size()-1)) {
            long unsigned a = function_address_mapping[name];
            long unsigned b = function_address_mapping[functions[i+1]];
            fn_size = (b-a);
        } else {
            long unsigned a = (long unsigned)(bytecode+function_address_mapping[name]);
            long unsigned b = (long unsigned)(bytecode+bytes);
            fn_size = (b-a);
        }

        oss << "; function size: " << fn_size << '\n';
        oss << ".def: " << name << " 1" << '\n';
        oss << ".end" << '\n';

        if (i < (functions.size()-1)) {
            oss << '\n';
        }

        disassembled_lines.push_back(oss.str());
    }

    for (unsigned i = 0; i < disassembled_lines.size(); ++i) {
        cout << disassembled_lines[i];
    }

    return 0;
}
