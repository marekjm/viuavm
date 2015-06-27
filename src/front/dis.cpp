#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>
#include <tuple>
#include <string>
#include "../version.h"
#include "../bytecode/opcodes.h"
#include "../bytecode/maps.h"
#include "../cg/disassembler/disassembler.h"
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


bool usage(const char* program, bool SHOW_HELP, bool SHOW_VERSION, bool VERBOSE) {
    if (SHOW_HELP or (SHOW_VERSION and VERBOSE)) {
        cout << "Viua VM disassembler, version ";
    }
    if (SHOW_HELP or SHOW_VERSION) {
        cout << VERSION << endl;
    }
    if (SHOW_HELP) {
        cout << "\nUSAGE:\n";
        cout << "    " << program << " [option...] [-o <outfile>] <infile>\n" << endl;
        cout << "OPTIONS:\n";
        cout << "    " << "-V, --version            - show version\n"
             << "    " << "-h, --help               - display this message\n"
             << "    " << "-v, --verbose            - show verbose output\n"
             << "    " << "-o, --out                - output to given path (by default prints to cout)\n"
             << "    " << "-i, --info               - include information about executable in output\n"
             << "    " << "-e, --with-entry         - include __entry function in disassembly\n"
             ;
    }

    return (SHOW_HELP or SHOW_VERSION);
}

int main(int argc, char* argv[]) {
    // setup command line arguments vector
    vector<string> args;
    string option;

    string filename = "";
    string disasmname = "";
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
        } else if (option == "--out" or option == "-o") {
            if (i < argc-1) {
                disasmname = string(argv[++i]);
            } else {
                cout << "error: option '" << argv[i] << "' requires an argument: filename" << endl;
                exit(1);
            }
            continue;
        } else {
            args.push_back(argv[i]);
        }
    }

    if (usage(argv[0], SHOW_HELP, SHOW_VERSION, VERBOSE)) { return 0; }

    if (args.size() == 0) {
        cout << "fatal: no input file" << endl;
        return 1;
    }

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
    map<string, unsigned> function_sizes;

    map<string, uint16_t> block_address_mapping = loader.getBlockAddresses();
    vector<string> blocks = loader.getBlocks();
    map<string, unsigned> block_sizes;


    vector<string> disassembled_lines;
    ostringstream oss;


    string name;
    unsigned fn_size;

    for (unsigned i = 0; i < functions.size(); ++i) {
        name = functions[i];

        if (i < (functions.size()-1)) {
            long unsigned a = function_address_mapping[name];
            long unsigned b = function_address_mapping[functions[i+1]];
            fn_size = (b-a);
        } else {
            long unsigned a = (long unsigned)(bytecode+function_address_mapping[name]);
            long unsigned b = (long unsigned)(bytecode+bytes);
            fn_size = (b-a);
        }

        function_sizes[name] = fn_size;
    }

    for (unsigned i = 0; i < blocks.size(); ++i) {
        name = blocks[i];

        if (i < (blocks.size()-1)) {
            long unsigned a = block_address_mapping[name];
            long unsigned b = block_address_mapping[blocks[i+1]];
            fn_size = (b-a);
        } else {
            long unsigned a = block_address_mapping[name];
            long unsigned b = function_address_mapping[functions[0]];
            fn_size = (b-a);
        }

        block_sizes[name] = fn_size;
    }

    if (INCLUDE_INFO) {
        oss << "; bytecode size: " << bytes << '\n';
        oss << ";\n";
        oss << "; functions:\n";
        for (unsigned i = 0; i < functions.size(); ++i) {
            name = functions[i];
            oss << ";   " << name << " -> " << function_sizes[name] << " bytes at byte " << function_address_mapping[functions[i]] << '\n';
        }
        oss << "\n\n";

        disassembled_lines.push_back(oss.str());
    }

    for (unsigned i = 0; i < blocks.size(); ++i) {
        name = blocks[i];
        fn_size = block_sizes[name];

        oss.str("");

        oss << ".block: " << name << '\n';

        string opname;
        bool disasm_terminated = false;
        for (unsigned j = 0; j < fn_size;) {
            try {
                string instruction;
                unsigned size;
                tie(instruction, size) = disassembler::instruction((bytecode+block_address_mapping[name]+j));
                oss << "    " << instruction << '\n';
                j += size;
            } catch (const out_of_range& e) {
                oss << "\n---- ERROR ----\n\n";
                oss << "disassembly terminated after throwing an instance of std::out_of_range\n";
                oss << "what(): " << e.what() << '\n';
                disasm_terminated = true;
                break;
            } catch (const string& e) {
                oss << "\n---- ERROR ----\n\n";
                oss << "disassembly terminated after throwing an instance of std::out_of_range\n";
                oss << "what(): " << e << '\n';
                disasm_terminated = true;
                break;
            }
        }
        if (disasm_terminated) {
            disassembled_lines.push_back(oss.str());
            break;
        }

        oss << ".end" << '\n';

        if (i < (blocks.size()-1)) {
            oss << '\n';
        }

        disassembled_lines.push_back(oss.str());
    }

    if (blocks.size()) {
        disassembled_lines.push_back("\n");
    }

    for (unsigned i = 0; i < functions.size(); ++i) {
        name = functions[i];
        fn_size = function_sizes[name];

        if ((name == "__entry") and not DISASSEMBLE_ENTRY) {
            continue;
        }

        oss.str("");

        oss << ".function: " << name << '\n';

        string opname;
        bool disasm_terminated = false;
        for (unsigned j = 0; j < fn_size;) {
            try {
                string instruction;
                unsigned size;
                tie(instruction, size) = disassembler::instruction((bytecode+function_address_mapping[name]+j));
                oss << "    " << instruction << '\n';
                j += size;
            } catch (const out_of_range& e) {
                oss << "\n---- ERROR ----\n\n";
                oss << "disassembly terminated after throwing an instance of std::out_of_range\n";
                oss << "what(): " << e.what() << '\n';
                disasm_terminated = true;
                break;
            } catch (const string& e) {
                oss << "\n---- ERROR ----\n\n";
                oss << "disassembly terminated after throwing an instance of std::out_of_range\n";
                oss << "what(): " << e << '\n';
                disasm_terminated = true;
                break;
            }
        }
        if (disasm_terminated) {
            disassembled_lines.push_back(oss.str());
            break;
        }

        oss << ".end" << '\n';

        if (i < (functions.size()-1)) {
            oss << '\n';
        }

        disassembled_lines.push_back(oss.str());
    }

    ostringstream assembly_code;
    for (unsigned i = 0; i < disassembled_lines.size(); ++i) {
        assembly_code << disassembled_lines[i];
    }

    if (disasmname.size()) {
        ofstream out(disasmname);
        out << assembly_code.str();
        out.close();
    } else {
        cout << assembly_code.str();
    }

    return 0;
}
