/*
 *  Copyright (C) 2015, 2016 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <fstream>
#include <viua/support/string.h>
#include <viua/support/env.h>
#include <viua/version.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/front/asm.h>
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
// are we only checking what size will the bytecode by?
bool REPORT_BYTECODE_SIZE = false;

bool VERBOSE = false;
bool DEBUG = false;
bool SCREAM = false;


static bool usage(const char* program, bool show_help, bool show_version, bool verbose) {
    if (show_help or (show_version and verbose)) {
        cout << "Viua VM assembler, version ";
    }
    if (show_help or show_version) {
        cout << VERSION << '.' << MICRO << endl;
    }
    if (show_help) {
        cout << "\nUSAGE:\n";
        cout << "    " << program << " [option...] [-o <outfile>] <infile> [<linked-file>...]\n" << endl;
        cout << "OPTIONS:\n";

        // generic options
        cout << "    " << "-V, --version            - show version\n"
             << "    " << "-h, --help               - display this message\n"

        // logging options
             << "    " << "-v, --verbose            - show verbose output\n"
             << "    " << "-d, --debug              - show debugging output\n"
             << "    " << "    --scream             - show so much debugging output it becomes noisy\n"

        // warning reporting level control
             << "    " << "-W, --Wall               - warn about everything\n"
             << "    " << "    --Wmissing-return    - warn about missing 'return' instruction at the end of functions\n"
             << "    " << "    --Wundefined-arity   - warn about functions declared with undefined arity\n"

        // error reporting level control
             << "    " << "-E, --Eall               - treat all warnings as errors\n"
             << "    " << "    --Emissing-return    - treat missing 'return' instruction at the end of function as error\n"
             << "    " << "    --Eundefined-arity   - treat functions declared with undefined arity as errors\n"
             << "    " << "    --Ehalt-is-last      - treat 'halt' being used as last instruction of 'main' function as error\n"

        // compilation options
             << "    " << "-o, --out <file>         - specify output file\n"
             << "    " << "-c, --lib                - assemble as a library\n"
             << "    " << "-e, --expand             - only expand the source code to simple form (one instruction per line)\n"
             << "    " << "                           with this option, assembler prints expanded source to standard output\n"
             << "    " << "-C, --verify             - verify source code correctness without actually compiling it\n"
             << "    " << "                           this option turns assembler into source level debugger and static code analyzer hybrid\n"
             << "    " << "    --size               - calculate and report final bytecode size\n";
             ;
    }

    return (show_help or show_version);
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
        } else if (option == "--out" or option == "-o") {
            if (i < argc-1) {
                compilename = string(argv[++i]);
            } else {
                cout << "error: option '" << argv[i] << "' requires an argument: filename" << endl;
                exit(1);
            }
            continue;
        } else if (option == "--lib" or option == "-c") {
            AS_LIB = true;
            continue;
        } else if (option == "--expand" or option == "-e") {
            EXPAND_ONLY = true;
            continue;
        } else if (option == "--verify" or option == "-C") {
            EARLY_VERIFICATION_ONLY = true;
            continue;
        } else if (option == "--size") {
            REPORT_BYTECODE_SIZE = true;
            continue;
        }else if (str::startswith(option, "-")) {
            cout << "error: unknown option: " << option << endl;
            return 1;
        }
        args.emplace_back(argv[i]);
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
            compilename = (filename + ".vlib");
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
        commandline_given_links.emplace_back(args[i]);
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
    while (getline(in, line)) { lines.emplace_back(line); }

    map<long unsigned, long unsigned> expanded_lines_to_source_lines;
    vector<string> expanded_lines = expandSource(lines, expanded_lines_to_source_lines);
    if (EXPAND_ONLY) {
        for (unsigned i = 0; i < expanded_lines.size(); ++i) {
            cout << expanded_lines[i] << endl;
        }
        return 0;
    }

    vector<string> ilines = assembler::ce::getilines(expanded_lines);
    invocables_t functions;
    if (gatherFunctions(&functions, expanded_lines, ilines)) {
        return 1;
    }
    invocables_t blocks;
    if (gatherBlocks(&blocks, expanded_lines, ilines)) {
        return 1;
    }

    ///////////////////////////////////////////
    // INITIAL VERIFICATION OF CODE CORRECTNESS
    try {
        assembler::verify::directives(expanded_lines);
        assembler::verify::instructions(expanded_lines);
        assembler::verify::ressInstructions(expanded_lines, AS_LIB);
        assembler::verify::functionNames(expanded_lines);
        assembler::verify::functionBodiesAreNonempty(expanded_lines);
        assembler::verify::blockTries(expanded_lines, blocks.names, blocks.signatures);
        assembler::verify::blockCatches(expanded_lines, blocks.names, blocks.signatures);
        assembler::verify::frameBalance(expanded_lines, expanded_lines_to_source_lines);
        assembler::verify::functionCallArities(expanded_lines);
        assembler::verify::msgArities(expanded_lines);
        assembler::verify::functionsEndWithReturn(expanded_lines);
        assembler::verify::blockBodiesAreNonempty(expanded_lines);
        assembler::verify::jumpsAreInRange(expanded_lines);
        assembler::verify::framesHaveOperands(expanded_lines);
        assembler::verify::framesHaveNoGaps(expanded_lines, expanded_lines_to_source_lines);
        assembler::verify::blocksEndWithFinishingInstruction(expanded_lines);
    } catch (const pair<unsigned, string>& e) {
        cout << filename << ':' << expanded_lines_to_source_lines.at(e.first)+1 << ": error: " << e.second << endl;
        return 1;
    }


    if (EARLY_VERIFICATION_ONLY) {
        return 0;
    }
    if (REPORT_BYTECODE_SIZE) {
        cout << Program::countBytes(ilines) << endl;
        return 0;
    }

    compilationflags_t flags;
    flags.as_lib = AS_LIB;
    flags.verbose = VERBOSE;
    flags.debug = DEBUG;
    flags.scream = SCREAM;

    int ret_code = 0;
    try {
        ret_code = generate(expanded_lines, ilines, functions, blocks, filename, compilename, commandline_given_links, flags);
    } catch (const string& e) {
        ret_code = 1;
        cout << "fatal: exception occured during assembling: " << e << endl;
    } catch (const char* e) {
        ret_code = 1;
        cout << "fatal: exception occured during assembling: " << e << endl;
    } catch (const pair<unsigned, string>& e) {
        cout << filename << ':' << expanded_lines_to_source_lines.at(e.first)+1 << ": error: " << e.second << endl;
        return 1;
    }

    return ret_code;
}
