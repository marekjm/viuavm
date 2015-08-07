#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <viua/bytecode/maps.h>
#include <viua/support/string.h>
#include <viua/support/env.h>
#include <viua/version.h>
#include <viua/loader.h>
#include <viua/program.h>
#include <viua/cg/assembler/assembler.h>
using namespace std;


// MISC FLAGS
bool SHOW_HELP = false;
bool SHOW_VERSION = false;

// are we assembling a library?
bool AS_LIB = false;

// are we just expanding the source to simple form?
bool EXPAND_ONLY = false;

bool VERBOSE = false;
bool DEBUG = false;
bool SCREAM = false;

bool WARNING_ALL = false;
bool ERROR_ALL = false;


// WARNINGS
bool WARNING_MISSING_END = false;
bool WARNING_EMPTY_FUNCTION_BODY = false;
bool WARNING_OPERANDLESS_FRAME = false;
bool WARNING_GLOBALS_IN_LIB = false;


// ERRORS
bool ERROR_MISSING_END = false;
bool ERROR_EMPTY_FUNCTION_BODY = false;
bool ERROR_OPERANDLESS_FRAME = false;
bool ERROR_GLOBALS_IN_LIB = false;


// ASSEMBLY CONSTANTS
/* const string ENTRY_FUNCTION_NAME = "__entry"; */


extern tuple<int, enum JUMPTYPE> resolvejump(string jmp, const map<string, int>& marks, int instruction_index = -1);

extern string resolveregister(string reg, const map<string, int>& names);


extern void assemble_three_intop_instruction(Program& program, map<string, int>& names, const string& instr, const string& operands);

extern vector<string> filter(const vector<string>& lines);

extern Program& compile(Program& program, const vector<string>& lines, map<string, int>& marks, map<string, int>& names);


extern void assemble(Program& program, const vector<string>& lines);


extern map<string, uint16_t> mapInvokableAddresses(uint16_t& starting_instruction, const vector<string>& names, const map<string, vector<string> >& sources);

extern unsigned extend(vector<string>& base, const vector<string>& v);


extern vector<string> tokenize(const string& s);

extern vector<vector<string>> decode_line_tokens(const vector<string>& tokens);
extern vector<vector<string>> decode_line(const string& s);

extern vector<string> expandSource(const vector<string>& lines);
extern int generate(const string& filename, string& compilename, const vector<string>& commandline_given_links);


bool usage(const char* program, bool SHOW_HELP, bool SHOW_VERSION, bool VERBOSE) {
    if (SHOW_HELP or (SHOW_VERSION and VERBOSE)) {
        cout << "Viua VM assembler, version ";
    }
    if (SHOW_HELP or SHOW_VERSION) {
        cout << VERSION << '.' << MICRO << ' ' << COMMIT << endl;
    }
    if (SHOW_HELP) {
        cout << "\nUSAGE:\n";
        cout << "    " << program << " [option...] [-o <outfile>] <infile> [<linked-file>...]\n" << endl;
        cout << "OPTIONS:\n";
        cout << "    " << "-V, --version            - show version\n"
             << "    " << "-h, --help               - display this message\n"
             << "    " << "-v, --verbose            - show verbose output\n"
             << "    " << "-d, --debug              - show debugging output\n"
             << "    " << "    --scream             - show so much debugging output it becomes noisy\n"
             << "    " << "-W, --Wall               - warn about everything\n"
             << "    " << "    --Wmissin-end        - warn about missing 'end' instruction at the end of functions\n"
             << "    " << "    --Wempty-function    - warn about empty functions\n"
             << "    " << "    --Wopless-frame      - warn about frames without operands\n"
             << "    " << "-E, --Eall               - treat all warnings as errors\n"
             << "    " << "    --Emissing-end       - treat missing 'end' instruction at the end of function as error\n"
             << "    " << "    --Eempty-function    - treat empty function as error\n"
             << "    " << "    --Eopless-frame      - treat frames without operands as errors\n"
             << "    " << "-c, --lib                - assemble as a library\n"
             << "    " << "    --expand             - only expand the source code to simple form (one instruction per line)\n"
             << "    " << "                           with this option, assembler prints expanded source to standard output\n"
             ;
    }

    return (SHOW_HELP or SHOW_VERSION);
}

int main(int argc, char* argv[]) {
    // setup command line arguments vector
    vector<string> args;
    string option;

    string filename(""), compilename("");

    for (int i = 1; i < argc; ++i) {
        option = string(argv[i]);
        if (option == "--help" or option == "-h") {
            SHOW_HELP = true;
            continue;
        } else if (option == "--version" or option == "-V") {
            SHOW_VERSION = true;
            continue;
        } else if (option == "--verbose" or option == "-v") {
            VERBOSE = true;
            continue;
        } else if (option == "--debug" or option == "-d") {
            DEBUG = true;
            continue;
        } else if (option == "--scream") {
            SCREAM = true;
            continue;
        } else if (option == "--lib" or option == "-c") {
            AS_LIB = true;
            continue;
        } else if (option == "--Wall" or option == "-W") {
            WARNING_ALL = true;
            continue;
        } else if (option == "--Eall" or option == "-E") {
            ERROR_ALL = true;
            continue;
        } else if (option == "--Wmissing-end") {
            WARNING_MISSING_END = true;
            continue;
        } else if (option == "--Wempty-function") {
            WARNING_EMPTY_FUNCTION_BODY = true;
            continue;
        } else if (option == "--Wopless-frame") {
            WARNING_OPERANDLESS_FRAME = true;
            continue;
        } else if (option == "--Wglobals-in-lib") {
            WARNING_GLOBALS_IN_LIB = true;
            continue;
        } else if (option == "--Emissing-end") {
            ERROR_MISSING_END = true;
            continue;
        } else if (option == "--Eempty-function") {
            ERROR_EMPTY_FUNCTION_BODY = true;
            continue;
        } else if (option == "--Eopless-frame") {
            ERROR_OPERANDLESS_FRAME = true;
            continue;
        } else if (option == "--Eglobals-in-lib") {
            ERROR_GLOBALS_IN_LIB = true;
            continue;
        } else if (option == "--out" or option == "-o") {
            if (i < argc-1) {
                compilename = string(argv[++i]);
            } else {
                cout << "error: option '" << argv[i] << "' requires an argument: filename" << endl;
                exit(1);
            }
            continue;
        } else if (option == "--expand") {
            EXPAND_ONLY = true;
            continue;
        }
        args.push_back(argv[i]);
    }

    if (usage(argv[0], SHOW_HELP, SHOW_VERSION, VERBOSE)) { return 0; }

    if (args.size() == 0) {
        cout << "fatal: no input file" << endl;
        return 1;
    }

    ////////////////////////////////
    // FIND FILENAME AND COMPILENAME
    filename = args[0];
    if (!filename.size()) {
        cout << "fatal: no file to assemble" << endl;
        return 1;
    }
    if (!support::env::isfile(filename)) {
        cout << "fatal: could not open file: " << filename << endl;
        return 1;
    }

    if (compilename == "") {
        if (AS_LIB) {
            compilename = (filename + ".wlib");
        } else {
            compilename = "a.out";
        }
    }

    if (VERBOSE or DEBUG) {
        cout << "message: assembling \"" << filename << "\" to \"" << compilename << "\"" << endl;
    }


    //////////////////////////////////////////
    // GATHER LINKS OBTAINED FROM COMMAND LINE
    vector<string> commandline_given_links;
    for (unsigned i = 1; i < args.size(); ++i) {
        commandline_given_links.push_back(args[i]);
    }

    int ret_code = generate(filename, compilename, commandline_given_links);

    return ret_code;
}
