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
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <unistd.h>
#include <vector>
#include <viua/assembler/util/pretty_printer.h>
#include <viua/bytecode/maps.h>
#include <viua/bytecode/opcodes.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/cg/disassembler/disassembler.h>
#include <viua/front/asm.h>
#include <viua/loader.h>
#include <viua/machine.h>
#include <viua/support/env.h>
#include <viua/support/pointer.h>
#include <viua/support/string.h>
#include <viua/version.h>
using namespace std;


using viua::assembler::util::pretty_printer::ATTR_RESET;
using viua::assembler::util::pretty_printer::COLOR_FG_RED;
using viua::assembler::util::pretty_printer::COLOR_FG_WHITE;
using viua::assembler::util::pretty_printer::send_control_seq;


// MISC FLAGS
bool SHOW_HELP    = false;
bool SHOW_VERSION = false;
bool VERBOSE      = false;
bool DEBUG        = false;

bool DISASSEMBLE_ENTRY        = false;
bool INCLUDE_INFO             = false;
bool LINE_BY_LINE             = false;
std::string SELECTED_FUNCTION = "";


static bool usage(const char* program,
                  bool show_help,
                  bool show_version,
                  bool verbose) {
    if (show_help or (show_version and verbose)) {
        cout << "Viua VM disassembler, version ";
    }
    if (show_help or show_version) {
        cout << VERSION << '.' << MICRO << endl;
    }
    if (show_help) {
        cout << "\nUSAGE:\n";
        cout << "    " << program << " [option...] [-o <outfile>] <infile>\n"
             << endl;
        cout << "OPTIONS:\n";
        cout << "    "
             << "-V, --version            - show version\n"
             << "    "
             << "-h, --help               - display this message\n"
             << "    "
             << "-v, --verbose            - show verbose output\n"
             << "    "
             << "-o, --out                - output to given path (by default "
                "prints to cout)\n"
             << "    "
             << "-i, --info               - include information about "
                "executable in output\n"
             << "    "
             << "-e, --with-entry         - include " << ENTRY_FUNCTION_NAME
             << " function in disassembly\n"
             << "    "
             << "-L, --line-by-line       - display output line by line\n"
             << "    "
             << "-F, --function <name>    - disassemble only selected "
                "function\n";
    }

    return (show_help or show_version);
}

int main(int argc, char* argv[]) {
    // setup command line arguments vector
    vector<std::string> args;
    std::string option;

    // for getline()
    std::string dummy;

    std::string filename   = "";
    std::string disasmname = "";
    for (int i = 1; i < argc; ++i) {
        option = std::string(argv[i]);
        if (option == "--help" or option == "-h") {
            SHOW_HELP = true;
        } else if (option == "--version" or option == "-V") {
            SHOW_VERSION = true;
        } else if (option == "--verbose" or option == "-v") {
            VERBOSE = true;
        } else if (option == "--debug") {
            DEBUG = true;
        } else if ((option == "--with-entry") or (option == "-e")) {
            DISASSEMBLE_ENTRY = true;
        } else if ((option == "--info") or (option == "-i")) {
            INCLUDE_INFO = true;
        } else if ((option == "--line-by-line") or (option == "-L")) {
            LINE_BY_LINE = true;
        } else if (option == "--function" or option == "-F") {
            if (i < argc - 1) {
                SELECTED_FUNCTION = std::string(argv[++i]);
            } else {
                cout << "error: option '" << argv[i]
                     << "' requires an argument: function name" << endl;
                exit(1);
            }
            continue;
        } else if (option == "--out" or option == "-o") {
            if (i < argc - 1) {
                disasmname = std::string(argv[++i]);
            } else {
                cout << "error: option '" << argv[i]
                     << "' requires an argument: filename" << endl;
                exit(1);
            }
            continue;
        } else if (str::startswith(option, "-")) {
            cerr << send_control_seq(COLOR_FG_RED) << "error"
                 << send_control_seq(ATTR_RESET);
            cerr << ": unknown option: ";
            cerr << send_control_seq(COLOR_FG_WHITE) << option
                 << send_control_seq(ATTR_RESET);
            cerr << endl;
            return 1;
        } else {
            args.emplace_back(argv[i]);
        }
    }

    if (usage(argv[0], SHOW_HELP, SHOW_VERSION, VERBOSE)) {
        return 0;
    }

    if (args.size() == 0) {
        cout << "fatal: no input file" << endl;
        return 1;
    }

    filename = args[0];

    if (!filename.size()) {
        cout << "fatal: no file to run" << endl;
        return 1;
    }
    if (!support::env::is_file(filename)) {
        cout << "fatal: could not open file: " << filename << endl;
        return 1;
    }

    Loader loader(filename);

    try {
        loader.executable();
    } catch (const std::string& e) {
        cout << e << endl;
        return 1;
    }

    uint64_t bytes = loader.get_bytecode_size();
    std::unique_ptr<viua::internals::types::byte[]> bytecode = loader.get_bytecode();

    map<std::string, uint64_t> function_address_mapping =
        loader.get_function_addresses();
    vector<std::string> functions             = loader.get_functions();
    map<std::string, uint64_t> function_sizes = loader.get_function_sizes();

    map<std::string, uint64_t> block_address_mapping =
        loader.get_block_addresses();
    vector<std::string> blocks = loader.get_blocks();
    map<std::string, uint64_t> block_sizes;

    map<std::string, uint64_t> element_address_mapping;
    vector<std::string> elements;
    map<std::string, uint64_t> element_sizes;
    map<std::string, string> element_types;

    vector<std::string> disassembled_lines;
    ostringstream oss;


    for (unsigned i = 0; i < blocks.size(); ++i) {
        const std::string name = blocks[i];

        uint64_t el_size = 0;
        if (i < (blocks.size() - 1)) {
            long unsigned a = block_address_mapping[name];
            long unsigned b = block_address_mapping[blocks[i + 1]];
            el_size         = (b - a);
        } else {
            long unsigned a = block_address_mapping[name];
            long unsigned b = function_address_mapping[functions[0]];
            el_size         = (b - a);
        }

        block_sizes[name] = el_size;

        element_sizes[name]           = el_size;
        element_types[name]           = "block";
        element_address_mapping[name] = block_address_mapping[name];
        elements.emplace_back(name);
    }

    for (unsigned i = 0; i < functions.size(); ++i) {
        const std::string name        = functions[i];
        element_sizes[name]           = function_sizes[name];
        element_types[name]           = "function";
        element_address_mapping[name] = function_address_mapping[name];
        elements.emplace_back(name);
    }

    if (INCLUDE_INFO) {
        (DEBUG ? cout : oss) << "-- bytecode size: " << bytes << '\n';
        (DEBUG ? cout : oss) << "--\n";
        (DEBUG ? cout : oss) << "-- functions:\n";
        for (unsigned i = 0; i < functions.size(); ++i) {
            const auto function_name = functions[i];
            (DEBUG ? cout : oss)
                << "--   " << function_name << " -> "
                << function_sizes[function_name] << " bytes at byte "
                << function_address_mapping[functions[i]] << '\n';
        }
        (DEBUG ? cout : oss) << "\n\n";

        if (not DEBUG) {
            disassembled_lines.emplace_back(oss.str());
        }
    }

    auto meta_information = loader.get_meta_information();
    if (meta_information.size()) {
        disassembled_lines.emplace_back("; meta information\n");
    }
    for (const auto each : meta_information) {
        disassembled_lines.emplace_back(
            assembler::utils::lines::make_info(each.first, each.second) + "\n");
    }
    if (meta_information.size()) {
        disassembled_lines.emplace_back("\n");
    }

    auto signatures = loader.get_external_signatures();
    if (signatures.size()) {
        disassembled_lines.emplace_back("; external function signatures\n");
    }
    for (const auto each : signatures) {
        disassembled_lines.emplace_back(
            assembler::utils::lines::make_function_signature(each) + "\n");
    }
    if (signatures.size()) {
        disassembled_lines.emplace_back("\n");
    }

    auto block_signatures = loader.get_external_block_signatures();
    if (block_signatures.size()) {
        disassembled_lines.emplace_back("; external block signatures\n");
    }
    for (const auto each : block_signatures) {
        disassembled_lines.emplace_back(
            assembler::utils::lines::make_block_signature(each) + "\n");
    }
    if (block_signatures.size()) {
        disassembled_lines.emplace_back("\n");
    }

    for (unsigned i = 0; i < elements.size(); ++i) {
        const std::string name = elements[i];
        const auto el_size     = element_sizes[name];

        if ((name == ENTRY_FUNCTION_NAME) and not DISASSEMBLE_ENTRY) {
            continue;
        }

        oss.str("");

        (DEBUG ? cout : oss)
            << '.' << element_types[name] << ": " << name << '\n';
        if (LINE_BY_LINE) {
            (DEBUG ? cout : oss) << '.' << element_types[name] << ": " << name;
            getline(cin, dummy);
        }

        std::string opname;
        bool disasm_terminated = false;
        for (unsigned j = 0; j < el_size;) {
            std::string instruction;
            try {
                unsigned size;
                tie(instruction, size) = disassembler::instruction(
                    (bytecode.get() + element_address_mapping[name] + j));
                if (DEBUG) {
                    if (j != 0) {
                        (DEBUG ? cout : oss) << '\n';
                    }
                    (DEBUG ? cout : oss)
                        << "    ; size: " << size << " bytes\n";
                    (DEBUG ? cout : oss)
                        << "    ; address: 0x" << hex << j << dec << '\n';
                }
                (DEBUG ? cout : oss) << "    " << instruction << '\n';
                j += size;
            } catch (const out_of_range& e) {
                (DEBUG ? cout : oss) << "\n---- ERROR ----\n\n";
                (DEBUG ? cout : oss) << "disassembly terminated after throwing "
                                        "an instance of std::out_of_range\n";
                (DEBUG ? cout : oss) << "what(): " << e.what() << '\n';
                disasm_terminated = true;
                break;
            } catch (const std::string& e) {
                (DEBUG ? cout : oss) << "\n---- ERROR ----\n\n";
                (DEBUG ? cout : oss) << "disassembly terminated after throwing "
                                        "an instance of std::out_of_range\n";
                (DEBUG ? cout : oss) << "what(): " << e << '\n';
                disasm_terminated = true;
                break;
            } catch (const char* e) {
                (DEBUG ? cout : oss) << "\n---- ERROR ----\n\n";
                (DEBUG ? cout : oss) << "disassembly terminated after throwing "
                                        "an instance of const char*\n";
                (DEBUG ? cout : oss) << "what(): " << e << '\n';
                disasm_terminated = true;
                break;
            }

            if (LINE_BY_LINE) {
                cout << "    " << instruction;
                getline(cin, dummy);
            }
        }
        if (disasm_terminated) {
            disassembled_lines.emplace_back(oss.str());
            break;
        }

        (DEBUG ? cout : oss) << ".end" << '\n';

        if (LINE_BY_LINE) {
            cout << ".end" << endl;
            getline(cin, dummy);
        }

        if (i < (elements.size() - 1 - (!DISASSEMBLE_ENTRY))) {
            (DEBUG ? cout : oss) << '\n';
        }

        if ((not SELECTED_FUNCTION.size()) or (SELECTED_FUNCTION == name)) {
            disassembled_lines.emplace_back(oss.str());
        }
    }

    if (DEBUG) {
        return 0;
    }

    ostringstream assembly_code;
    for (unsigned i = 0; i < disassembled_lines.size(); ++i) {
        assembly_code << disassembled_lines[i];
    }


    // do not print full dump if line-by-line
    if (LINE_BY_LINE) {
        return 0;
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
