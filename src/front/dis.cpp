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


using viua::assembler::util::pretty_printer::ATTR_RESET;
using viua::assembler::util::pretty_printer::COLOR_FG_RED;
using viua::assembler::util::pretty_printer::COLOR_FG_WHITE;
using viua::assembler::util::pretty_printer::send_control_seq;


// MISC FLAGS
bool SHOW_HELP    = false;
bool SHOW_VERSION = false;
bool VERBOSE      = false;
bool DEBUG        = false;

bool DISASSEMBLE_ENTRY = false;
bool INCLUDE_INFO      = false;
bool LINE_BY_LINE      = false;
auto SELECTED_FUNCTION = std::string{};


static bool usage(const char* program,
                  bool show_help,
                  bool show_version,
                  bool verbose) {
    if (show_help or (show_version and verbose)) {
        std::cout << "Viua VM disassembler, version ";
    }
    if (show_help or show_version) {
        std::cout << VERSION << '.' << MICRO << std::endl;
    }
    if (show_help) {
        std::cout << "\nUSAGE:\n";
        std::cout << "    " << program << " [option...] [-o <outfile>] <infile>\n"
             << std::endl;
        std::cout << "OPTIONS:\n";
        std::cout << "    "
             << "-V, --version            - show version\n"
             << "    "
             << "-h, --help               - display this message\n"
             << "    "
             << "-v, --verbose            - show verbose output\n"
             << "    "
             << "-o, --out                - output to given path (by default "
                "prints to std::cout)\n"
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
    auto args   = std::vector<std::string>{};
    auto option = std::string{};

    // for getline()
    auto dummy = std::string{};

    auto filename   = std::string{""};
    auto disasmname = std::string{};
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
                std::cout << "error: option '" << argv[i]
                     << "' requires an argument: function name" << std::endl;
                exit(1);
            }
            continue;
        } else if (option == "--out" or option == "-o") {
            if (i < argc - 1) {
                disasmname = std::string(argv[++i]);
            } else {
                std::cout << "error: option '" << argv[i]
                     << "' requires an argument: filename" << std::endl;
                exit(1);
            }
            continue;
        } else if (str::startswith(option, "-")) {
            std::cerr << send_control_seq(COLOR_FG_RED) << "error"
                 << send_control_seq(ATTR_RESET);
            std::cerr << ": unknown option: ";
            std::cerr << send_control_seq(COLOR_FG_WHITE) << option
                 << send_control_seq(ATTR_RESET);
            std::cerr << std::endl;
            return 1;
        } else {
            args.emplace_back(argv[i]);
        }
    }

    if (usage(argv[0], SHOW_HELP, SHOW_VERSION, VERBOSE)) {
        return 0;
    }

    if (args.size() == 0) {
        std::cout << "fatal: no input file" << std::endl;
        return 1;
    }

    filename = args[0];

    if (!filename.size()) {
        std::cout << "fatal: no file to run" << std::endl;
        return 1;
    }
    if (!viua::support::env::is_file(filename)) {
        std::cout << "fatal: could not open file: " << filename << std::endl;
        return 1;
    }

    Loader loader(filename);

    try {
        loader.executable();
    } catch (std::string const& e) {
        std::cout << e << std::endl;
        return 1;
    }

    uint64_t bytes = loader.get_bytecode_size();
    std::unique_ptr<viua::internals::types::byte[]> bytecode =
        loader.get_bytecode();

    std::map<std::string, uint64_t> function_address_mapping =
        loader.get_function_addresses();
    std::vector<std::string> functions        = loader.get_functions();
    std::map<std::string, uint64_t> function_sizes = loader.get_function_sizes();

    std::map<std::string, uint64_t> block_address_mapping =
        loader.get_block_addresses();
    std::vector<std::string> blocks = loader.get_blocks();
    std::map<std::string, uint64_t> block_sizes;

    std::map<std::string, uint64_t> element_address_mapping;
    auto elements = std::vector<std::string>{};
    std::map<std::string, uint64_t> element_sizes;
    std::map<std::string, std::string> element_types;

    auto disassembled_lines = std::vector<std::string>{};
    std::ostringstream oss;


    for (unsigned i = 0; i < blocks.size(); ++i) {
        std::string const name = blocks[i];

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
        std::string const name        = functions[i];
        element_sizes[name]           = function_sizes[name];
        element_types[name]           = "function";
        element_address_mapping[name] = function_address_mapping[name];
        elements.emplace_back(name);
    }

    if (INCLUDE_INFO) {
        (DEBUG ? std::cout : oss) << "-- bytecode size: " << bytes << '\n';
        (DEBUG ? std::cout : oss) << "--\n";
        (DEBUG ? std::cout : oss) << "-- functions:\n";
        for (unsigned i = 0; i < functions.size(); ++i) {
            const auto function_name = functions[i];
            (DEBUG ? std::cout : oss)
                << "--   " << function_name << " -> "
                << function_sizes[function_name] << " bytes at byte "
                << function_address_mapping[functions[i]] << '\n';
        }
        (DEBUG ? std::cout : oss) << "\n\n";

        if (not DEBUG) {
            disassembled_lines.emplace_back(oss.str());
        }
    }

    auto meta_information = loader.get_meta_information();
    if (meta_information.size()) {
        disassembled_lines.emplace_back("; meta information\n");
    }
    for (auto const& each : meta_information) {
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
    for (auto const& each : signatures) {
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
    for (auto const& each : block_signatures) {
        disassembled_lines.emplace_back(
            assembler::utils::lines::make_block_signature(each) + "\n");
    }
    if (block_signatures.size()) {
        disassembled_lines.emplace_back("\n");
    }

    {
        auto dynamic_imports = loader.dynamic_imports();
        if (dynamic_imports.size()) {
            disassembled_lines.emplace_back("; dynamic imports\n");
        }
        for (auto const& each : dynamic_imports) {
            disassembled_lines.emplace_back(
                ".import: [[dynamic]] " + each + "\n");
        }
        if (dynamic_imports.size()) {
            disassembled_lines.emplace_back("\n");
        }
    }

    for (unsigned i = 0; i < elements.size(); ++i) {
        std::string const name = elements[i];
        const auto el_size     = element_sizes[name];

        if ((name == ENTRY_FUNCTION_NAME) and not DISASSEMBLE_ENTRY) {
            continue;
        }

        oss.str("");

        (DEBUG ? std::cout : oss)
            << '.' << element_types[name] << ": " << name << '\n';
        if (LINE_BY_LINE) {
            (DEBUG ? std::cout : oss) << '.' << element_types[name] << ": " << name;
            getline(std::cin, dummy);
        }

        auto opname            = std::string{};
        bool disasm_terminated = false;
        for (unsigned j = 0; j < el_size;) {
            auto instruction = std::string{};
            try {
                unsigned size;
                tie(instruction, size) = disassembler::instruction(
                    (bytecode.get() + element_address_mapping[name] + j));
                if (DEBUG) {
                    if (j != 0) {
                        (DEBUG ? std::cout : oss) << '\n';
                    }
                    (DEBUG ? std::cout : oss)
                        << "    ; size: " << size << " bytes\n";
                    (DEBUG ? std::cout : oss)
                        << "    ; address: 0x" << std::hex << j << std::dec << '\n';
                }
                (DEBUG ? std::cout : oss) << "    " << instruction << '\n';
                j += size;
            } catch (std::out_of_range const& e) {
                (DEBUG ? std::cout : oss) << "\n---- ERROR ----\n\n";
                (DEBUG ? std::cout : oss) << "disassembly terminated after throwing "
                                        "an instance of std::out_of_range\n";
                (DEBUG ? std::cout : oss) << "what(): " << e.what() << '\n';
                disasm_terminated = true;
                break;
            } catch (std::string const& e) {
                (DEBUG ? std::cout : oss) << "\n---- ERROR ----\n\n";
                (DEBUG ? std::cout : oss) << "disassembly terminated after throwing "
                                        "an instance of std::out_of_range\n";
                (DEBUG ? std::cout : oss) << "what(): " << e << '\n';
                disasm_terminated = true;
                break;
            } catch (const char* e) {
                (DEBUG ? std::cout : oss) << "\n---- ERROR ----\n\n";
                (DEBUG ? std::cout : oss) << "disassembly terminated after throwing "
                                        "an instance of const char*\n";
                (DEBUG ? std::cout : oss) << "what(): " << e << '\n';
                disasm_terminated = true;
                break;
            }

            if (LINE_BY_LINE) {
                std::cout << "    " << instruction;
                std::getline(std::cin, dummy);
            }
        }
        if (disasm_terminated) {
            disassembled_lines.emplace_back(oss.str());
            break;
        }

        (DEBUG ? std::cout : oss) << ".end" << '\n';

        if (LINE_BY_LINE) {
            std::cout << ".end" << std::endl;
            std::getline(std::cin, dummy);
        }

        if (i < (elements.size() - 1 - (!DISASSEMBLE_ENTRY))) {
            (DEBUG ? std::cout : oss) << '\n';
        }

        if ((not SELECTED_FUNCTION.size()) or (SELECTED_FUNCTION == name)) {
            disassembled_lines.emplace_back(oss.str());
        }
    }

    if (DEBUG) {
        return 0;
    }

    std::ostringstream assembly_code;
    for (unsigned i = 0; i < disassembled_lines.size(); ++i) {
        assembly_code << disassembled_lines[i];
    }


    // do not print full dump if line-by-line
    if (LINE_BY_LINE) {
        return 0;
    }
    if (disasmname.size()) {
        std::ofstream out(disasmname);
        out << assembly_code.str();
        out.close();
    } else {
        std::cout << assembly_code.str();
    }

    return 0;
}
