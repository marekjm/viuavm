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

#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <viua/version.h>
#include <viua/bytecode/maps.h>
#include <viua/cg/disassembler/disassembler.h>
#include <viua/program.h>
#include <viua/printutils.h>
#include <viua/front/vm.h>
using namespace std;


const char* NOTE_LOADED_ASM = "note: seems like you have loaded an .asm file which cannot be run on CPU without prior compilation";


// MISC FLAGS
bool SHOW_HELP = false;
bool SHOW_VERSION = false;
bool VERBOSE = false;


bool usage(const char* program, bool show_help, bool show_version, bool verbose) {
    if (show_help or (show_version and verbose)) {
        cout << "Viua VM CPU, version ";
    }
    if (show_help or show_version) {
        cout << VERSION << '.' << MICRO << endl;
    }
    if (show_help) {
        cout << "\nUSAGE:\n";
        cout << "    " << program << " [option...] <executable>\n" << endl;
        cout << "OPTIONS:\n";
        cout << "    " << "-V, --version            - show version\n"
             << "    " << "-h, --help               - display this message\n"
             << "    " << "-v, --verbose            - show verbose output\n"
             ;
    }

    return (show_help or show_version);
}

int main(int argc, char* argv[]) {
    // setup command line arguments vector
    vector<string> args;
    string option;
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
        } else if (str::startswith(option, "-")) {
            cout << "error: unknown option: " << option << endl;
            return 1;
        }
        args.push_back(argv[i]);
    }

    if (usage(argv[0], SHOW_HELP, SHOW_VERSION, VERBOSE)) { return 0; }

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
    if (!support::env::isfile(filename)) {
        cout << "fatal: could not open file: " << filename << endl;
        return 1;
    }

    CPU cpu;

    try {
        viua::front::vm::initialise(&cpu, filename, args);
    } catch (const char *e) {
        cout << "error: " << e << endl;
        return 1;
    } catch (const string& e) {
        cout << "error: " << e << endl;
        return 1;
    }

    try {
        // try preloading dynamic libraries specified by environment
        viua::front::vm::preload_libraries(&cpu);
    } catch (const Exception* e) {
        cout << "fatal: preload: " << e->what() << endl;
        return 1;
    }

    viua::front::vm::load_standard_prototypes(&cpu);

    try {
        cpu.run();
    } catch (const Exception* e) {
        cout << "VM error: an irrecoverable VM exception occured: " << e->what() << endl;
        return 1;
    } catch (const std::exception& e) {
        cout << "VM error: an irrecoverable host exception occured: " << e.what() << endl;
        return 1;
    }
    // the catch (...) is intentionally omitted, if we can't provide useful information about
    // the error it's better to just crash

    return cpu.exit();
}
