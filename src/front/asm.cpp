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

#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <viua/support/string.h>
#include <viua/support/env.h>
#include <viua/version.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/cg/lex.h>
#include <viua/cg/tools.h>
#include <viua/front/asm.h>
using namespace std;


// MISC FLAGS
bool SHOW_HELP = false;
bool SHOW_VERSION = false;

// are we assembling a library?
bool AS_LIB = false;

// are we only verifying source code correctness?
bool EARLY_VERIFICATION_ONLY = false;
// are we only checking what size will the bytecode by?
bool REPORT_BYTECODE_SIZE = false;
bool PERFORM_STATIC_ANALYSIS = true;
bool SHOW_META = false;

bool VERBOSE = false;
bool DEBUG = false;
bool SCREAM = false;


string send_control_seq(const string& mode) {
    static auto is_terminal = isatty(1);
    static string env_color_flag { getenv("VIUAVM_ASM_COLOUR") ? getenv("VIUAVM_ASM_COLOUR") : "default" };

    bool colorise = is_terminal;
    if (env_color_flag == "default") {
        // do nothing; the default is to colorise when printing to teminal and
        // do not colorise otherwise
    } else if (env_color_flag == "never") {
        colorise = false;
    } else if (env_color_flag == "always") {
        colorise = true;
    } else {
        // unknown value, do nothing
    }

    if (colorise) {
        return mode;
    }
    return "";
}


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
             << "    " << "-C, --verify             - verify source code correctness without actually compiling it\n"
             << "    " << "                           this option turns assembler into source level debugger and static code analyzer hybrid\n"
             << "    " << "    --size               - calculate and report final bytecode size\n"
             << "    " << "    --meta               - display information embedded in source code and exit\n"
             << "    " << "    --no-sa              - disable static checking of register accesses (use in case of false positives)\n"
             ;
    }

    return (show_help or show_version);
}

static string read_file(const string& path) {
    ifstream in(path, ios::in | ios::binary);

    ostringstream source_in;
    string line;
    while (getline(in, line)) { source_in << line << '\n'; }

    return source_in.str();
}

static void underline_error_token(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i, const viua::cg::lex::InvalidSyntax& error) {
        cout << "     ";

        auto len = str::stringify((error.line()+1), false).size();
        while (len--) {
            cout << ' ';
        }
        cout << ' ';

        while (i < tokens.size()) {
            const auto& each = tokens.at(i++);
            bool match = error.match(each);

            if (match) {
                cout << send_control_seq(COLOR_FG_RED_1);
            }

            char c = (match ? '^' : ' ');
            len = each.str().size();
            while (len--) {
                cout << c;
            }

            if (match) {
                cout << send_control_seq(ATTR_RESET);
            }

            if (each == "\n") {
                break;
            }
        }

        cout << '\n';
}
static auto display_error_line(const vector<viua::cg::lex::Token>& tokens, const viua::cg::lex::InvalidSyntax& error, decltype(tokens.size()) i) -> decltype(i) {
    const auto token_line = tokens.at(i).line();

    cout << send_control_seq(COLOR_FG_RED);
    cout << ">>>>"; // message indent, ">>>>" on error line
    cout << ' ';

    cout << send_control_seq(COLOR_FG_YELLOW);
    cout << token_line+1;
    cout << ' ';

    auto original_i = i;

    cout << send_control_seq(COLOR_FG_WHITE);
    while (i < tokens.size() and tokens.at(i).line() == token_line) {
        bool highlighted = false;
        if (error.match(tokens.at(i))) {
            cout << send_control_seq(COLOR_FG_ORANGE_RED_1);
            highlighted = true;
        }
        cout << tokens.at(i++).str();
        if (highlighted) {
            cout << send_control_seq(COLOR_FG_WHITE);
        }
    }

    cout << send_control_seq(ATTR_RESET);

    underline_error_token(tokens, original_i, error);

    return i;
}
static auto display_context_line(const vector<viua::cg::lex::Token>& tokens, const viua::cg::lex::InvalidSyntax&, decltype(tokens.size()) i) -> decltype(i) {
    const auto token_line = tokens.at(i).line();

    cout << "    "; // message indent, ">>>>" on error line
    cout << ' ';
    cout << token_line+1;
    cout << ' ';

    while (i < tokens.size() and tokens.at(i).line() == token_line) {
        cout << tokens.at(i++).str();
    }

    return i;
}
static void display_error_header(const viua::cg::lex::InvalidSyntax& error, const string& filename) {
    cout << send_control_seq(COLOR_FG_WHITE) << filename << ':' << error.line()+1 << ':' << error.character()+1 << ':' << send_control_seq(ATTR_RESET) << ' ';
    cout << send_control_seq(COLOR_FG_RED) << "error" << send_control_seq(ATTR_RESET) << ": " << error.what() << endl;
}
static void display_error_location(const vector<viua::cg::lex::Token>& tokens, const viua::cg::lex::InvalidSyntax error) {
    const unsigned context_lines = 2;
    decltype(error.line()) context_before = 0, context_after = (error.line()+context_lines);
    if (error.line() >= context_lines) {
        context_before = (error.line()-context_lines);
    }

    for (std::remove_reference<decltype(tokens)>::type::size_type i = 0; i < tokens.size();) {
        if (tokens.at(i).line() > context_after) {
            break;
        }
        if (tokens.at(i).line() >= context_before) {
            if (tokens.at(i).line() == error.line()) {
                i = display_error_line(tokens, error, i);
            } else {
                i = display_context_line(tokens, error, i);
            }
            continue;
        }
        ++i;
    }
}
static void display_error_in_context(const vector<viua::cg::lex::Token>& tokens, const viua::cg::lex::InvalidSyntax error, const string& filename) {
    display_error_header(error, filename);
    cout << "\n";
    display_error_location(tokens, error);
}
static void display_error_in_context(const vector<viua::cg::lex::Token>& tokens, const viua::cg::lex::TracedSyntaxError error, const string& filename) {
    for (auto const& e : error.errors) {
        display_error_in_context(tokens, e, filename);
        cout << "\n";
    }
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
                cout << send_control_seq(COLOR_FG_RED) << "error" << send_control_seq(ATTR_RESET);
                cout << ": option '" << send_control_seq(COLOR_FG_WHITE) << argv[i] << send_control_seq(ATTR_RESET) << "' requires an argument: filename";
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
        } else if (str::startswith(option, "-")) {
            cout << send_control_seq(COLOR_FG_RED) << "error" << send_control_seq(ATTR_RESET);
            cout << ": unknown option: ";
            cout << send_control_seq(COLOR_FG_WHITE) << option << send_control_seq(ATTR_RESET);
            cout << endl;
            return 1;
        }
        args.emplace_back(argv[i]);
    }

    if (usage(argv[0], SHOW_HELP, SHOW_VERSION, VERBOSE)) { return 0; }

    if (args.size() == 0) {
        cout << send_control_seq(COLOR_FG_RED) << "error" << send_control_seq(ATTR_RESET);
        cout << ": no input file" << endl;
        return 1;
    }

    ////////////////////////////////
    // FIND FILENAME AND COMPILENAME
    filename = args[0];
    if (!filename.size()) {
        cout << send_control_seq(COLOR_FG_RED) << "error" << send_control_seq(ATTR_RESET);
        cout << ": no file to assemble" << endl;
        return 1;
    }
    if (!support::env::isfile(filename)) {
        cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET) << ": ";
        cout << send_control_seq(COLOR_FG_RED) << "error" << send_control_seq(ATTR_RESET);
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
        cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << "message" << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "assembling to \"";
        cout << send_control_seq(COLOR_FG_WHITE) << compilename << send_control_seq(ATTR_RESET);
        cout << "\"\n";
    }


    //////////////////////////////////////////
    // GATHER LINKS OBTAINED FROM COMMAND LINE
    vector<string> commandline_given_links;
    for (unsigned i = 1; i < args.size(); ++i) {
        commandline_given_links.emplace_back(args[i]);
    }


    auto source = read_file(filename);
    auto raw_tokens = viua::cg::lex::tokenise(source);
    decltype(raw_tokens) cooked_tokens, cooked_tokens_without_names_replaced;
    try {
        cooked_tokens = viua::cg::lex::standardise(viua::cg::lex::cook(raw_tokens));
        cooked_tokens_without_names_replaced = viua::cg::lex::standardise(viua::cg::lex::cook(raw_tokens, false));
    } catch (const viua::cg::lex::InvalidSyntax& e) {
        display_error_in_context(raw_tokens, e, filename);
        return 1;
    } catch (const viua::cg::lex::TracedSyntaxError& e) {
        display_error_in_context(raw_tokens, e, filename);
        return 1;
    }

    invocables_t functions;
    try {
        if (gatherFunctions(&functions, cooked_tokens)) {
            return 1;
        }
    } catch (const viua::cg::lex::InvalidSyntax& e) {
        display_error_in_context(raw_tokens, e, filename);
        return 1;
    } catch (const viua::cg::lex::TracedSyntaxError& e) {
        display_error_in_context(raw_tokens, e, filename);
        return 1;
    }

    invocables_t blocks;
    try {
        if (gatherBlocks(&blocks, cooked_tokens)) {
            return 1;
        }
    } catch (const viua::cg::lex::InvalidSyntax& e) {
        display_error_in_context(raw_tokens, e, filename);
        return 1;
    } catch (const viua::cg::lex::TracedSyntaxError& e) {
        display_error_in_context(raw_tokens, e, filename);
        return 1;
    }

    ///////////////////////////////////////////
    // INITIAL VERIFICATION OF CODE CORRECTNESS
    try {
        assembler::verify::directives(cooked_tokens_without_names_replaced);
        assembler::verify::instructions(cooked_tokens_without_names_replaced);
        assembler::verify::ressInstructions(cooked_tokens_without_names_replaced, AS_LIB);
        assembler::verify::functionNames(cooked_tokens_without_names_replaced);
        assembler::verify::functionBodiesAreNonempty(cooked_tokens_without_names_replaced);
        assembler::verify::blockTries(cooked_tokens_without_names_replaced, blocks.names, blocks.signatures);
        assembler::verify::blockCatches(cooked_tokens_without_names_replaced, blocks.names, blocks.signatures);
        assembler::verify::frameBalance(cooked_tokens_without_names_replaced);
        assembler::verify::functionCallArities(cooked_tokens_without_names_replaced);
        assembler::verify::msgArities(cooked_tokens_without_names_replaced);
        assembler::verify::functionsEndWithReturn(cooked_tokens_without_names_replaced);
        assembler::verify::blockBodiesAreNonempty(cooked_tokens_without_names_replaced);
        assembler::verify::jumpsAreInRange(cooked_tokens_without_names_replaced);
        assembler::verify::framesHaveNoGaps(cooked_tokens_without_names_replaced);
        assembler::verify::blocksEndWithFinishingInstruction(cooked_tokens_without_names_replaced);
        if (PERFORM_STATIC_ANALYSIS) {
            assembler::verify::manipulationOfDefinedRegisters(cooked_tokens_without_names_replaced, blocks.tokens, DEBUG);
        }
    } catch (const viua::cg::lex::InvalidSyntax& e) {
        display_error_in_context(raw_tokens, e, filename);
        return 1;
    } catch (const viua::cg::lex::TracedSyntaxError& e) {
        display_error_in_context(raw_tokens, e, filename);
        return 1;
    }


    if (EARLY_VERIFICATION_ONLY) {
        return 0;
    }
    if (REPORT_BYTECODE_SIZE) {
        cout << viua::cg::tools::calculate_bytecode_size2(cooked_tokens) << endl;
        return 0;
    }

    compilationflags_t flags;
    flags.as_lib = AS_LIB;
    flags.verbose = VERBOSE;
    flags.debug = DEBUG;
    flags.scream = SCREAM;

    if (SHOW_META) {
        auto meta = gatherMetaInformation(cooked_tokens);
        for (auto each : meta) {
            cout << each.first << " = " << str::enquote(str::strencode(each.second)) << endl;
        }
        return 0;
    }

    int ret_code = 0;
    try {
        ret_code = generate(cooked_tokens, functions, blocks, filename, compilename, commandline_given_links, flags);
    } catch (const string& e) {
        ret_code = 1;
        cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET) << ": ";
        cout << send_control_seq(COLOR_FG_RED) << "error" << send_control_seq(ATTR_RESET);
        cout << ": " << e << endl;
    } catch (const char* e) {
        ret_code = 1;
        cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET) << ": ";
        cout << send_control_seq(COLOR_FG_RED) << "error" << send_control_seq(ATTR_RESET);
        cout << ": " << e << endl;
    } catch (const viua::cg::lex::InvalidSyntax& e) {
        display_error_in_context(raw_tokens, e, filename);
        return 1;
    } catch (const viua::cg::lex::TracedSyntaxError& e) {
        display_error_in_context(raw_tokens, e, filename);
        return 1;
    }

    return ret_code;
}
