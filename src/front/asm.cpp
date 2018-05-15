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

#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <viua/assembler/frontend/static_analyser.h>
#include <viua/assembler/util/pretty_printer.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/cg/lex.h>
#include <viua/cg/tools.h>
#include <viua/front/asm.h>
#include <viua/support/env.h>
#include <viua/support/string.h>
#include <viua/version.h>
using namespace std;


using viua::assembler::util::pretty_printer::ATTR_RESET;
using viua::assembler::util::pretty_printer::COLOR_FG_LIGHT_GREEN;
using viua::assembler::util::pretty_printer::COLOR_FG_RED;
using viua::assembler::util::pretty_printer::COLOR_FG_WHITE;
using viua::assembler::util::pretty_printer::send_control_seq;


// MISC FLAGS
bool SHOW_HELP    = false;
bool SHOW_VERSION = false;

// are we assembling a library?
bool AS_LIB = false;

// are we only verifying source code correctness?
bool EARLY_VERIFICATION_ONLY = false;
// are we only checking what size will the bytecode by?
bool REPORT_BYTECODE_SIZE    = false;
bool PERFORM_STATIC_ANALYSIS = true;
bool USE_NEW_SA              = true;
bool SHOW_META               = false;

bool VERBOSE = false;
bool DEBUG   = false;
bool SCREAM  = false;


static bool usage(const char* program,
                  bool show_help,
                  bool show_version,
                  bool verbose) {
    if (show_help or (show_version and verbose)) {
        cout << "Viua VM assembler, version ";
    }
    if (show_help or show_version) {
        cout << VERSION << '.' << MICRO << endl;
    }
    if (show_help) {
        cout << "\nUSAGE:\n";
        cout << "    " << program
             << " [option...] [-o <outfile>] <infile> [<linked-file>...]\n"
             << endl;
        cout << "OPTIONS:\n";

        // generic options
        cout << "    "
             << "-V, --version            - show version\n"
             << "    "
             << "-h, --help               - display this message\n"

             // logging options
             << "    "
             << "-v, --verbose            - show verbose output\n"
             << "    "
             << "-d, --debug              - show debugging output\n"
             << "    "
             << "    --scream             - show so much debugging output it "
                "becomes noisy\n"

             // warning reporting level control
             << "    "
             << "-W, --Wall               - warn about everything\n"
             << "    "
             << "    --Wmissing-return    - warn about missing 'return' "
                "instruction at the end of functions\n"
             << "    "
             << "    --Wundefined-arity   - warn about functions declared with "
                "undefined arity\n"

             // error reporting level control
             << "    "
             << "-E, --Eall               - treat all warnings as errors\n"
             << "    "
             << "    --Emissing-return    - treat missing 'return' instruction "
                "at the end of function as "
                "error\n"
             << "    "
             << "    --Eundefined-arity   - treat functions declared with "
                "undefined arity as errors\n"
             << "    "
             << "    --Ehalt-is-last      - treat 'halt' being used as last "
                "instruction of 'main' function "
                "as error\n"

             // compilation options
             << "    "
             << "-o, --out <file>         - specify output file\n"
             << "    "
             << "-c, --lib                - assemble as a library\n"
             << "    "
             << "-C, --verify             - verify source code correctness "
                "without actually compiling it\n"
             << "    "
             << "                           this option turns assembler into "
                "source level debugger and "
                "static code analyzer hybrid\n"
             << "    "
             << "    --size               - calculate and report final "
                "bytecode size\n"
             << "    "
             << "    --meta               - display information embedded in "
                "source code and exit\n"
             << "    "
             << "    --no-sa              - disable static checking of "
                "register accesses (use in case of "
                "false positives)\n"
             << "    --new-sa             - use new static analyser (more "
                "precise, with better features, but "
                "without coverage of all instructions yet)\n";
    }

    return (show_help or show_version);
}

static std::string read_file(std::string const& path) {
    ifstream in(path, ios::in | ios::binary);

    ostringstream source_in;
    auto line = std::string{};
    while (getline(in, line)) {
        source_in << line << '\n';
    }

    return source_in.str();
}

int main(int argc, char* argv[]) {
    // setup command line arguments std::vector
    auto args   = std::vector<std::string>{};
    auto option = std::string{};

    std::string filename(""), compilename("");

    for (int i = 1; i < argc; ++i) {
        option = std::string(argv[i]);

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
            if (i < argc - 1) {
                compilename = std::string(argv[++i]);
            } else {
                cout << send_control_seq(COLOR_FG_RED) << "error"
                     << send_control_seq(ATTR_RESET);
                cout << ": option '" << send_control_seq(COLOR_FG_WHITE)
                     << argv[i] << send_control_seq(ATTR_RESET)
                     << "' requires an argument: filename";
                cout << endl;
                exit(1);
            }
            continue;
        } else if (option == "--lib" or option == "-c") {
            AS_LIB = true;
            continue;
        } else if (option == "--verify" or option == "-C") {
            EARLY_VERIFICATION_ONLY = true;
            continue;
        } else if (option == "--size") {
            REPORT_BYTECODE_SIZE = true;
            continue;
        } else if (option == "--meta") {
            SHOW_META = true;
            continue;
        } else if (option == "--no-sa") {
            PERFORM_STATIC_ANALYSIS = false;
            continue;
        } else if (option == "--new-sa") {
            USE_NEW_SA = true;
            continue;
        } else if (str::startswith(option, "-")) {
            cerr << send_control_seq(COLOR_FG_RED) << "error"
                 << send_control_seq(ATTR_RESET);
            cerr << ": unknown option: ";
            cerr << send_control_seq(COLOR_FG_WHITE) << option
                 << send_control_seq(ATTR_RESET);
            cerr << endl;
            return 1;
        }
        args.emplace_back(argv[i]);
    }

    if (usage(argv[0], SHOW_HELP, SHOW_VERSION, VERBOSE)) {
        return 0;
    }

    if (args.size() == 0) {
        cout << send_control_seq(COLOR_FG_RED) << "error"
             << send_control_seq(ATTR_RESET);
        cout << ": no input file" << endl;
        return 1;
    }

    ////////////////////////////////
    // FIND FILENAME AND COMPILENAME
    filename = args[0];
    if (!filename.size()) {
        cout << send_control_seq(COLOR_FG_RED) << "error"
             << send_control_seq(ATTR_RESET);
        cout << ": no file to assemble" << endl;
        return 1;
    }
    if (!viua::support::env::is_file(filename)) {
        cout << send_control_seq(COLOR_FG_WHITE) << filename
             << send_control_seq(ATTR_RESET) << ": ";
        cout << send_control_seq(COLOR_FG_RED) << "error"
             << send_control_seq(ATTR_RESET);
        cout << ": could not open file" << endl;
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
        cout << send_control_seq(COLOR_FG_WHITE) << filename
             << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << "message"
             << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "assembling to \"";
        cout << send_control_seq(COLOR_FG_WHITE) << compilename
             << send_control_seq(ATTR_RESET);
        cout << "\"\n";
    }


    //////////////////////////////////////////
    // GATHER LINKS OBTAINED FROM COMMAND LINE
    auto commandline_given_links = std::vector<std::string>{};
    for (unsigned i = 1; i < args.size(); ++i) {
        commandline_given_links.emplace_back(args[i]);
    }


    auto source     = read_file(filename);
    auto raw_tokens = viua::cg::lex::tokenise(source);
    decltype(raw_tokens) cooked_tokens, cooked_tokens_without_names_replaced,
        normalised_tokens;
    try {
        cooked_tokens =
            viua::cg::lex::standardise(viua::cg::lex::cook(raw_tokens));
        cooked_tokens_without_names_replaced =
            viua::cg::lex::standardise(viua::cg::lex::cook(raw_tokens, false));
        normalised_tokens =
            viua::cg::lex::normalise(viua::cg::lex::cook(raw_tokens));
    } catch (viua::cg::lex::Invalid_syntax const& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(
            raw_tokens, e, filename);
        return 1;
    } catch (viua::cg::lex::Traced_syntax_error const& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(
            raw_tokens, e, filename);
        return 1;
    }

    auto functions = viua::front::assembler::invocables_t{};
    try {
        functions = viua::front::assembler::gather_functions(cooked_tokens);
    } catch (viua::cg::lex::Invalid_syntax const& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(
            raw_tokens, e, filename);
        return 1;
    } catch (viua::cg::lex::Traced_syntax_error const& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(
            raw_tokens, e, filename);
        return 1;
    }

    auto blocks = viua::front::assembler::invocables_t{};
    try {
        blocks = viua::front::assembler::gather_blocks(cooked_tokens);
    } catch (viua::cg::lex::Invalid_syntax const& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(
            raw_tokens, e, filename);
        return 1;
    } catch (viua::cg::lex::Traced_syntax_error const& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(
            raw_tokens, e, filename);
        return 1;
    }

    ///////////////////////////////////////////
    // INITIAL VERIFICATION OF CODE CORRECTNESS
    try {
        auto parsed_source =
            viua::assembler::frontend::parser::parse(normalised_tokens);
        parsed_source.as_library = AS_LIB;
        viua::assembler::frontend::static_analyser::verify(parsed_source);
        if (PERFORM_STATIC_ANALYSIS) {
            if (USE_NEW_SA) {
                viua::assembler::frontend::static_analyser::
                    check_register_usage(parsed_source);
            } else {
                assembler::verify::manipulation_of_defined_registers(
                    cooked_tokens_without_names_replaced, blocks.tokens, DEBUG);
            }
        }
    } catch (viua::cg::lex::Invalid_syntax const& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(
            raw_tokens, e, filename);
        return 1;
    } catch (viua::cg::lex::Traced_syntax_error const& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(
            raw_tokens, e, filename);
        return 1;
    }


    if (EARLY_VERIFICATION_ONLY) {
        return 0;
    }
    if (REPORT_BYTECODE_SIZE) {
        cout << viua::cg::tools::calculate_bytecode_size2(cooked_tokens)
             << endl;
        return 0;
    }

    auto flags    = viua::front::assembler::compilationflags_t{};
    flags.as_lib  = AS_LIB;
    flags.verbose = VERBOSE;
    flags.debug   = DEBUG;
    flags.scream  = SCREAM;

    if (SHOW_META) {
        auto meta =
            viua::front::assembler::gather_meta_information(cooked_tokens);
        for (auto each : meta) {
            cout << each.first << " = "
                 << str::enquote(str::strencode(each.second)) << endl;
        }
        return 0;
    }

    int ret_code = 0;
    try {
        generate(cooked_tokens,
                 functions,
                 blocks,
                 filename,
                 compilename,
                 commandline_given_links,
                 flags);
    } catch (std::string const& e) {
        ret_code = 1;
        cout << send_control_seq(COLOR_FG_WHITE) << filename
             << send_control_seq(ATTR_RESET) << ": ";
        cout << send_control_seq(COLOR_FG_RED) << "error"
             << send_control_seq(ATTR_RESET);
        cout << ": " << e << endl;
    } catch (const char* e) {
        ret_code = 1;
        cout << send_control_seq(COLOR_FG_WHITE) << filename
             << send_control_seq(ATTR_RESET) << ": ";
        cout << send_control_seq(COLOR_FG_RED) << "error"
             << send_control_seq(ATTR_RESET);
        cout << ": " << e << endl;
    } catch (viua::cg::lex::Invalid_syntax const& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(
            raw_tokens, e, filename);
        return 1;
    } catch (viua::cg::lex::Traced_syntax_error const& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(
            raw_tokens, e, filename);
        return 1;
    }

    return ret_code;
}
