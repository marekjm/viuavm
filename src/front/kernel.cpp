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


const char* NOTE_LOADED_ASM = "note: seems like you have loaded an .asm file which cannot be run on Kernel without prior compilation";


static bool usage(const string program, const vector<string>& args) {
    bool show_help = false;
    bool show_version = false;
    bool verbose = false;
    bool show_info = false;
    bool show_json = false;

    for (auto option : args) {
        if (option == "--help" or option == "-h") {
            show_help = true;
            continue;
        } else if (option == "--version" or option == "-V") {
            show_version = true;
            continue;
        } else if (option == "--verbose" or option == "-v") {
            verbose = true;
            continue;
        } else if (option == "--info" or option == "-i") {
            show_info = true;
            continue;
        } else if (option == "--json") {
            show_json = true;
        } else if (str::startswith(option, "-")) {
            cout << "error: unknown option: " << option << endl;
            exit(1);
        }
    }

    if (show_json) {
        cout << "{\"version\": \"" << VERSION << '.' << MICRO << "\", \"sched\": {\"ffi\": " << Kernel::no_of_ffi_schedulers() << ", ";
        cout << "\"vp\": " << Kernel::no_of_vp_schedulers() << "}}\n";
        return true;
    }

    if (show_help or (show_version and verbose)) {
        cout << "Viua VM Kernel, version ";
    }
    if (show_help or show_version or show_info) {
        cout << VERSION << '.' << MICRO;
        if (not show_info) {
            cout << endl;
        }
    }
    if (show_info) {
        cout << ' ';
        cout << "[sched:ffi=" << Kernel::no_of_ffi_schedulers() << ']';
        cout << ' ';
        cout << "[sched:vp=" << Kernel::no_of_vp_schedulers() << ']' << endl;
    }
    if (show_help) {
        cout << "\nUSAGE:\n";
        cout << "    " << program << " [option...] <executable>\n" << endl;
        cout << "OPTIONS:\n";
        cout << "    " << "-V, --version            - show version\n"
             << "    " << "-h, --help               - display this message\n"
             << "    " << "-v, --verbose            - show verbose output\n"
             << "    " << "-i, --info               - show information about VM configuration (number of schedulers, version etc.)\n"
             << "    " << "    --json               - same as --info but in JSON format\n"
             ;
    }

    return (show_help or show_version or show_info);
}

int main(int argc, char* argv[]) {
    // setup command line arguments vector
    vector<string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    if (usage(argv[0], args)) { return 0; }

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

    Kernel kernel;

    try {
        viua::front::vm::initialise(&kernel, filename, args);
    } catch (const char *e) {
        cout << "error: " << e << endl;
        return 1;
    } catch (const string& e) {
        cout << "error: " << e << endl;
        return 1;
    }

    try {
        // try preloading dynamic libraries specified by environment
        viua::front::vm::preload_libraries(&kernel);
    } catch (const viua::types::Exception* e) {
        cout << "fatal: preload: " << e->what() << endl;
        return 1;
    }

    viua::front::vm::load_standard_prototypes(&kernel);

    try {
        kernel.run();
    } catch (const viua::types::Exception* e) {
        cout << "VM error: an irrecoverable VM exception occured: " << e->what() << endl;
        return 1;
    } catch (const std::exception& e) {
        cout << "VM error: an irrecoverable host exception occured: " << e.what() << endl;
        return 1;
    }
    // the catch (...) is intentionally omitted, if we can't provide useful information about
    // the error it's better to just crash

    return kernel.exit();
}
