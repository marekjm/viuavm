/*
 *  Copyright (C) 2017 Marek Marecki
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
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <viua/assembler/frontend/parser.h>
#include <viua/assembler/util/pretty_printer.h>
#include <viua/bytecode/maps.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/cg/lex.h>
#include <viua/cg/tools.h>
#include <viua/support/env.h>
#include <viua/support/string.h>
#include <viua/version.h>
using namespace std;


// MISC FLAGS
bool SHOW_HELP = false;
bool SHOW_VERSION = false;
bool VERBOSE = false;

bool AS_LIB = false;


using namespace viua::assembler::frontend::parser;
using Token = viua::cg::lex::Token;
using InvalidSyntax = viua::cg::lex::InvalidSyntax;


template<class T>
static auto enumerate(const vector<T>& v) -> vector<pair<typename vector<T>::size_type, T>> {
    vector<pair<typename vector<T>::size_type, T>> enumerated_vector;

    typename vector<T>::size_type i = 0;
    for (const auto& each : v) {
        enumerated_vector.emplace_back(i, each);
        ++i;
    }

    return enumerated_vector;
}

static bool usage(const char* program, bool show_help, bool show_version, bool verbose) {
    if (show_help or (show_version and verbose)) {
        cout << "Viua VM lexer, version ";
    }
    if (show_help or show_version) {
        cout << VERSION << '.' << MICRO << endl;
    }
    if (show_help) {
        cout << "\nUSAGE:\n";
        cout << "    " << program << " [option...] <infile>\n" << endl;
        cout << "OPTIONS:\n";

        // generic options
        cout << "    "
             << "-V, --version            - show version\n"
             << "    "
             << "-h, --help               - display this message\n"
             << "    ";

        // compilation options
        cout << "-c, --lib                - assemble as a library\n";
    }

    return (show_help or show_version);
}

static string read_file(ifstream& in) {
    ostringstream source_in;
    string line;
    while (getline(in, line)) {
        source_in << line << '\n';
    }

    return source_in.str();
}

static auto invalid_syntax(const vector<Token>& tokens, const string message) -> InvalidSyntax {
    auto invalid_syntax_error = InvalidSyntax(tokens.at(0), message);
    for (auto i = std::remove_reference_t<decltype(tokens)>::size_type{1}; i < tokens.size(); ++i) {
        invalid_syntax_error.add(tokens.at(i));
    }
    return invalid_syntax_error;
}

using Verifier = auto (*)(const ParsedSource&, const InstructionsBlock&) -> void;

static auto verify_wrapper(const ParsedSource& source, Verifier verifier) -> void {
    for (const auto& fn : source.functions) {
        try {
            verifier(source, fn);
        } catch (InvalidSyntax& e) {
            throw viua::cg::lex::TracedSyntaxError().append(e).append(
                InvalidSyntax(fn.name, ("in function " + fn.name.str())));
        }
    }
    for (const auto& bl : source.blocks) {
        try {
            verifier(source, bl);
        } catch (InvalidSyntax& e) {
            throw viua::cg::lex::TracedSyntaxError().append(e).append(
                InvalidSyntax(bl.name, ("in block " + bl.name.str())));
        }
    }
}
static auto verify_ress_instructions(const ParsedSource& src) -> void {
    verify_wrapper(src, [](const ParsedSource& source, const InstructionsBlock& ib) -> void {
        for (const auto& line : ib.body) {
            const auto instruction =
                dynamic_cast<viua::assembler::frontend::parser::Instruction*>(line.get());
            if (not instruction) {
                continue;
            }
            if (instruction->opcode != RESS) {
                continue;
            }
            const auto label =
                dynamic_cast<viua::assembler::frontend::parser::Label*>(instruction->operands.at(0).get());
            if (not label) {
                throw invalid_syntax(instruction->operands.at(0)->tokens,
                                     "illegal operand for 'ress' instruction");
            }
            if (not(label->content == "global" or label->content == "static" or label->content == "local")) {
                throw invalid_syntax(instruction->operands.at(0)->tokens, "not a register set name");
            }
            if (label->content == "global" and source.as_library) {
                throw invalid_syntax(instruction->operands.at(0)->tokens,
                                     "global register set used by a library function");
            }
        }
    });
}
static auto is_defined_block_name(const ParsedSource& source, const string name) -> bool {
    auto is_undefined = (source.blocks.end() ==
                         find_if(source.blocks.begin(), source.blocks.end(),
                                 [name](const InstructionsBlock& ib) -> bool { return (ib.name == name); }));
    if (is_undefined) {
        is_undefined =
            (source.block_signatures.end() ==
             find_if(source.block_signatures.begin(), source.block_signatures.end(),
                     [name](const Token& block_name) -> bool { return (block_name.str() == name); }));
    }
    return (not is_undefined);
}
static auto verify_block_tries(const ParsedSource& src) -> void {
    verify_wrapper(src, [](const ParsedSource& source, const InstructionsBlock& ib) -> void {
        for (const auto& line : ib.body) {
            auto instruction = dynamic_cast<viua::assembler::frontend::parser::Instruction*>(line.get());
            if (not instruction) {
                continue;
            }
            if (instruction->opcode != ENTER) {
                continue;
            }
            auto block_name = instruction->tokens.at(1);

            if (not is_defined_block_name(source, block_name)) {
                throw InvalidSyntax(block_name, ("cannot enter undefined block: " + block_name.str()));
            }
        }
    });
}
static auto verify_block_catches(const ParsedSource& src) -> void {
    verify_wrapper(src, [](const ParsedSource& source, const InstructionsBlock& ib) -> void {
        for (const auto& line : ib.body) {
            auto instruction = dynamic_cast<viua::assembler::frontend::parser::Instruction*>(line.get());
            if (not instruction) {
                continue;
            }
            if (instruction->opcode != CATCH) {
                continue;
            }
            auto block_name = instruction->tokens.at(2);

            if (not is_defined_block_name(source, block_name)) {
                throw InvalidSyntax(block_name, ("cannot catch using undefined block: " + block_name.str()))
                    .add(instruction->tokens.at(0));
            }
        }
    });
}
static auto verify_block_endings(const ParsedSource& src) -> void {
    verify_wrapper(src, [](const ParsedSource&, const InstructionsBlock& ib) -> void {
        auto last_instruction =
            dynamic_cast<viua::assembler::frontend::parser::Instruction*>(ib.body.back().get());
        if (not last_instruction) {
            throw invalid_syntax(ib.body.back()->tokens, "invalid end of block: expected mnemonic");
        }
        auto opcode = last_instruction->opcode;
        if (not(opcode == RETURN or opcode == TAILCALL or opcode == HALT)) {
            throw viua::cg::lex::InvalidSyntax(
                last_instruction->tokens.at(0),
                "invalid last mnemonic: expected one of: return, tailcall or halt");
        }
    });
}
static auto verify(const ParsedSource& source) -> void {
    verify_ress_instructions(source);
    verify_block_tries(source);
    verify_block_catches(source);
    verify_block_endings(source);
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
        } else if (option == "--lib" or option == "-c") {
            AS_LIB = true;
            continue;
        } else if (str::startswith(option, "-")) {
            cerr << "error: unknown option: " << option << endl;
            return 1;
        }
        args.emplace_back(argv[i]);
    }

    if (usage(argv[0], SHOW_HELP, SHOW_VERSION, VERBOSE)) {
        return 0;
    }

    if (args.size() == 0) {
        cout << "fatal: no input file" << endl;
        return 1;
    }

    ////////////////////////////////
    // FIND FILENAME AND COMPILENAME
    filename = args[0];
    if (!filename.size()) {
        cout << "fatal: no file to tokenize" << endl;
        return 1;
    }
    if (!support::env::isfile(filename)) {
        cout << "fatal: could not open file: " << filename << endl;
        return 1;
    }

    ////////////////
    // READ LINES IN
    ifstream in(filename, ios::in | ios::binary);
    if (!in) {
        cout << "fatal: file could not be opened: " << filename << endl;
        return 1;
    }

    string source = read_file(in);

    auto raw_tokens = viua::cg::lex::tokenise(source);
    vector<Token> tokens;
    vector<Token> normalised_tokens;

    try {
        tokens = viua::cg::lex::cook(raw_tokens);
        normalised_tokens = normalise(tokens);
    } catch (const viua::cg::lex::InvalidSyntax& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(raw_tokens, e, filename);
        return 1;
    } catch (const viua::cg::lex::TracedSyntaxError& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(raw_tokens, e, filename);
        return 1;
    }

    try {
        auto parsed_source = viua::assembler::frontend::parser::parse(normalised_tokens);
        parsed_source.as_library = AS_LIB;
        verify(parsed_source);
    } catch (const viua::cg::lex::InvalidSyntax& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(raw_tokens, e, filename);
        return 1;
    } catch (const viua::cg::lex::TracedSyntaxError& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(raw_tokens, e, filename);
        return 1;
    }

    return 0;
}
