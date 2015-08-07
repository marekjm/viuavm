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
// are we only verifying source code correctness?
bool EARLY_VERIFICATION_ONLY = false;

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


extern vector<string> expandSource(const vector<string>&);
extern int generate(const vector<string>&, string&, string&, const vector<string>&);


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
             << "    " << "    --verify             - verify source code correctness without actually compiling it\n"
             << "    " << "                         - verify source code correctness without actually compiling it\n"
             << "    " << "                           this option turns assembler into source level debugger and static code analyzer hybrid\n"
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
        } else if (option == "--verify") {
            EARLY_VERIFICATION_ONLY = true;
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


    ////////////////
    // READ LINES IN
    ifstream in(filename, ios::in | ios::binary);
    if (!in) {
        cout << "fatal: file could not be opened: " << filename << endl;
        return 1;
    }

    vector<string> lines;
    string line;
    while (getline(in, line)) { lines.push_back(line); }

    vector<string> expanded_lines = expandSource(lines);
    if (EXPAND_ONLY) {
        for (unsigned i = 0; i < expanded_lines.size(); ++i) {
            cout << expanded_lines[i] << endl;
        }
        return 0;
    }


    ///////////////////////////////////////////
    // INITIAL VERIFICATION OF CODE CORRECTNESS
    string report;
    if ((report = assembler::verify::directives(expanded_lines)).size()) {
        cout << report << endl;
        return 1;
    }
    if ((report = assembler::verify::instructions(expanded_lines)).size()) {
        cout << report << endl;
        return 1;
    }
    if ((report = assembler::verify::ressInstructions(expanded_lines, AS_LIB)).size()) {
        cout << report << endl;
        return 1;
    }

    if (EARLY_VERIFICATION_ONLY) {
        return 0;
    }

    int ret_code = generate(expanded_lines, filename, compilename, commandline_given_links);

    return ret_code;
}
