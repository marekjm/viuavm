#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <tuple>
#include <string>
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


// Helper functions for checking if a container contains an item.
template<typename T> bool in(std::vector<T> v, T item) {
    return (find(v.begin(), v.end(), item) != v.end());
}
template<typename K, typename V> bool in(std::map<K, V> m, K key) {
    return (m.count() == 1);
}


// Helper functions for disassembler
tuple<string, unsigned> disline(byte* ptr) {
    byte* bptr = ptr;

    OPCODE op = OPCODE(*bptr);
    string opname = OP_NAMES.at(op);

    ++bptr; // instruction byte is not needed anymore

    ostringstream oss;
    oss << opname;
    if (in(OP_VARIABLE_LENGTH, op)) {
        bptr += string(bptr).size();
        ++bptr; // for null character terminating the C-style string not included in std::string
        bptr += (OP_SIZES.at(opname)-1); // -1 because OP_SIZES add one for instruction-storing byte
    } else {
        bptr += (OP_SIZES.at(opname)-1); // -1 because OP_SIZES add one for instruction-storing byte
    }

    unsigned increase = (bptr-ptr);

    switch (op) {
        case IZERO:
        case PRINT:
        case ECHO:
        case NOT:
            ++ptr;

            oss << " ";
            oss << (*((bool*)ptr) ? "@" : "");
            pointer::inc<bool, byte>(ptr);
            oss << *(int*)ptr;
            pointer::inc<int, byte>(ptr);

            break;
        case ISTORE:
            ++ptr;

            oss << " ";
            oss << (*((bool*)ptr) ? "@" : "");
            pointer::inc<bool, byte>(ptr);
            oss << *(int*)ptr;
            pointer::inc<int, byte>(ptr);

            oss << " ";
            oss << (*((bool*)ptr) ? "@" : "");
            pointer::inc<bool, byte>(ptr);
            oss << *(int*)ptr;
            pointer::inc<int, byte>(ptr);

            break;
        case IADD:
        case ISUB:
        case IMUL:
        case IDIV:
        case IINC:
        case IDEC:
        case ILT:
        case ILTE:
        case IGT:
        case IGTE:
        case IEQ:
            ++ptr;

            oss << " ";
            oss << (*((bool*)ptr) ? "@" : "");
            pointer::inc<bool, byte>(ptr);
            oss << *(int*)ptr;
            pointer::inc<int, byte>(ptr);

            oss << " ";
            oss << (*((bool*)ptr) ? "@" : "");
            pointer::inc<bool, byte>(ptr);
            oss << *(int*)ptr;
            pointer::inc<int, byte>(ptr);

            oss << " ";
            oss << (*((bool*)ptr) ? "@" : "");
            pointer::inc<bool, byte>(ptr);
            oss << *(int*)ptr;
            pointer::inc<int, byte>(ptr);

            break;
        default:
            oss << "";
    }
    return tuple<string, unsigned>(oss.str(), increase);
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
    map<string, unsigned> function_sizes;

    map<string, uint16_t> block_address_mapping = loader.getBlockAddresses();
    vector<string> blocks = loader.getBlocks();


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

    for (unsigned i = 0; i < functions.size(); ++i) {
        name = functions[i];
        fn_size = function_sizes[name];

        if ((name == "__entry") and not DISASSEMBLE_ENTRY) {
            continue;
        }

        oss.str("");

        oss << ".def: " << name << " 1" << '\n';

        string opname;
        bool disasm_terminated = false;
        for (unsigned j = 0; j < fn_size;) {
            try {
                string instruction;
                unsigned size;
                tie(instruction, size) = disline((bytecode+function_address_mapping[name]+j));
                oss << "    " << instruction << '\n';
                j += size;
            } catch (const out_of_range& e) {
                oss << "\n---- ERROR ----\n\n";
                oss << "disassembly terminated after throwing an instance of std::out_of_range\n";
                oss << "what(): " << e.what() << '\n';
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

    for (unsigned i = 0; i < disassembled_lines.size(); ++i) {
        cout << disassembled_lines[i];
    }

    return 0;
}
