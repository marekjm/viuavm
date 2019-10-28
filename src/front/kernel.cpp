/*
 *  Copyright (C) 2015, 2016, 2017, 2018 Marek Marecki
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

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <viua/assembler/util/pretty_printer.h>
#include <viua/bytecode/maps.h>
#include <viua/cg/disassembler/disassembler.h>
#include <viua/front/asm.h>
#include <viua/front/vm.h>
#include <viua/printutils.h>
#include <viua/program.h>
#include <viua/version.h>


using viua::assembler::util::pretty_printer::ATTR_RESET;
using viua::assembler::util::pretty_printer::COLOR_FG_RED;
using viua::assembler::util::pretty_printer::COLOR_FG_WHITE;
using viua::assembler::util::pretty_printer::send_control_seq;


const char* NOTE_LOADED_ASM = "note: seems like you have loaded an .asm file "
                              "which cannot be run without prior compilation";


static auto display_vm_information(bool const verbose) -> void {
    auto const full_version = std::string{VERSION} + "." + MICRO;
    auto const proc_schedulers = viua::kernel::Kernel::no_of_process_schedulers();
    auto const ffi_schedulers = viua::kernel::Kernel::no_of_ffi_schedulers();
    auto const io_schedulers = viua::kernel::Kernel::no_of_io_schedulers();
    auto const cpus_available = std::thread::hardware_concurrency();

    std::cerr << "Viua VM " << full_version << " [";

    if (verbose) { std::cerr << "hardware="; }
    std::cerr << cpus_available << ':';

    if (verbose) { std::cerr << "proc="; }
    std::cerr << proc_schedulers << ':';

    if (verbose) { std::cerr << "ffi="; }
    std::cerr << ffi_schedulers << ':';

    if (verbose) { std::cerr << "io="; }
    std::cerr << io_schedulers;

    std::cerr << "]\n";
}
static bool usage(std::string const program,
                  std::vector<std::string> const& args) {
    bool show_help    = false;
    bool show_version = false;
    bool verbose      = false;
    bool show_info    = false;

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
        } else if (str::startswith(option, "-")) {
            std::cerr << send_control_seq(COLOR_FG_RED) << "error"
                 << send_control_seq(ATTR_RESET);
            std::cerr << ": unknown option: ";
            std::cerr << send_control_seq(COLOR_FG_WHITE) << option
                 << send_control_seq(ATTR_RESET);
            std::cerr << "\n";
            exit(1);
        } else {
            // first operand, options processing should stop
            break;
        }
    }

    if (show_info) {
        display_vm_information(verbose);
        return true;
    }

    if (show_help or (show_version and verbose)) {
        std::cout << "Viua VM kernel version ";
    }
    if (show_help or show_version) {
        std::cout << VERSION << '.' << MICRO << "\n";
    }
    if (show_help) {
        std::cout << "\nUSAGE:\n";
        std::cout << "    " << program << " [option...] <executable>\n" << std::endl;
        std::cout << "OPTIONS:\n";
        std::cout
            << "    "
            << "-V, --version            - show version\n"
            << "    "
            << "-h, --help               - display this message\n"
            << "    "
            << "-v, --verbose            - show verbose output\n"
            << "    "
            << "-i, --info               - show information about VM "
               "configuration (number of schedulers, "
               "version etc.)\n"
            << "    "
            << "    --json               - same as --info but in JSON format\n";
    }

    return (show_help or show_version);
}

int main(int argc, char* argv[]) {
    // setup command line arguments std::vector
    auto args = std::vector<std::string>{};
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    if (usage(argv[0], args)) {
        return 0;
    }

    if (args.size() == 0) {
        std::cerr << "fatal: no input file\n";
        return 1;
    }

    auto filename = std::string{};
    filename      = args[0];

    if (!filename.size()) {
        std::cout << "fatal: no file to run\n";
        return 1;
    }
    if (!viua::support::env::is_file(filename)) {
        std::cout << "fatal: could not open file: " << filename << "\n";
        return 1;
    }

    viua::kernel::Kernel kernel;

    try {
        viua::front::vm::initialise(kernel, filename, args);
    } catch (std::unique_ptr<viua::types::Exception> const& e) {
        std::cerr << "fatal: kernel initialisation fault: " << e->what()
                  << '\n';
        return 1;
    } catch (const char* e) {
        std::cout << "error: " << e << std::endl;
        return 1;
    } catch (std::string const& e) {
        std::cout << "error: " << e << std::endl;
        return 1;
    }

    try {
        // try preloading dynamic libraries specified by environment
        viua::front::vm::preload_libraries(kernel);
    } catch (std::unique_ptr<viua::types::Exception> const& e) {
        std::cout << "fatal: preload: " << e->what() << std::endl;
        return 1;
    }

    try {
        kernel.run();
    } catch (std::unique_ptr<viua::types::Exception> const& e) {
        std::cout << "VM error: an irrecoverable VM exception occured: " << e->what()
             << std::endl;
        return 1;
    } catch (std::exception const& e) {
        std::cout << "VM error: an irrecoverable host exception occured: "
             << e.what() << std::endl;
        return 1;
    }
    // the catch (...) is intentionally omitted, if we can't provide useful
    // information about the error it's better to just crash

    return kernel.exit();
}
