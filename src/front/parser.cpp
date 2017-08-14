/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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
#include <unistd.h>
#include <utility>
#include <vector>
#include <viua/bytecode/maps.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/cg/lex.h>
#include <viua/cg/tools.h>
#include <viua/front/asm.h>
#include <viua/support/env.h>
#include <viua/support/string.h>
#include <viua/version.h>
using namespace std;


// MISC FLAGS
bool SHOW_HELP = false;
bool SHOW_VERSION = false;
bool VERBOSE = false;


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


string send_control_seq(const string& mode) {
    static auto is_terminal = isatty(1);
    static string env_color_flag{getenv("VIUAVM_ASM_COLOUR") ? getenv("VIUAVM_ASM_COLOUR") : "default"};

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
             // misc options
             << "    "
             << "    --size               - calculate and display compiled bytecode size\n"
             << "    "
             << "    --raw                - dump raw token list\n"
             << "    "
             << "    --ws                 - reduce whitespace and remove comments\n"
             << "    "
             << "    --dirs               - reduce directives\n";
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

static void underline_error_token(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i,
                                  const viua::cg::lex::InvalidSyntax& error) {
    cout << "     ";

    auto len = str::stringify((error.line() + 1), false).size();
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
static auto display_error_line(const vector<viua::cg::lex::Token>& tokens,
                               const viua::cg::lex::InvalidSyntax& error, decltype(tokens.size()) i)
    -> decltype(i) {
    const auto token_line = tokens.at(i).line();

    cout << send_control_seq(COLOR_FG_RED);
    cout << ">>>>";  // message indent, ">>>>" on error line
    cout << ' ';

    cout << send_control_seq(COLOR_FG_YELLOW);
    cout << token_line + 1;
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
static auto display_context_line(const vector<viua::cg::lex::Token>& tokens,
                                 const viua::cg::lex::InvalidSyntax&, decltype(tokens.size()) i)
    -> decltype(i) {
    const auto token_line = tokens.at(i).line();

    cout << "    ";  // message indent, ">>>>" on error line
    cout << ' ';
    cout << token_line + 1;
    cout << ' ';

    while (i < tokens.size() and tokens.at(i).line() == token_line) {
        cout << tokens.at(i++).str();
    }

    return i;
}
static void display_error_header(const viua::cg::lex::InvalidSyntax& error, const string& filename) {
    cout << send_control_seq(COLOR_FG_WHITE) << filename << ':' << error.line() + 1 << ':'
         << error.character() + 1 << ':' << send_control_seq(ATTR_RESET) << ' ';
    cout << send_control_seq(COLOR_FG_RED) << "error" << send_control_seq(ATTR_RESET) << ": " << error.what()
         << endl;
}
static void display_error_location(const vector<viua::cg::lex::Token>& tokens,
                                   const viua::cg::lex::InvalidSyntax error) {
    const unsigned context_lines = 2;
    decltype(error.line()) context_before = 0, context_after = (error.line() + context_lines);
    if (error.line() >= context_lines) {
        context_before = (error.line() - context_lines);
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
static void display_error_in_context(const vector<viua::cg::lex::Token>& tokens,
                                     const viua::cg::lex::InvalidSyntax error, const string& filename) {
    display_error_header(error, filename);
    cout << "\n";
    display_error_location(tokens, error);
}
static void display_error_in_context(const vector<viua::cg::lex::Token>& tokens,
                                     const viua::cg::lex::TracedSyntaxError error, const string& filename) {
    for (auto const& e : error.errors) {
        display_error_in_context(tokens, e, filename);
        cout << "\n";
    }
}

template<typename T> class vector_view {
    const vector<T>& vec;
    const typename std::remove_reference_t<decltype(vec)>::size_type offset;

  public:
    using size_type = decltype(offset);

    auto at(const decltype(offset) i) const -> const T& { return vec.at(offset + i); }
    auto size() const -> size_type { return vec.size(); }

    vector_view(const decltype(vec) v, const decltype(offset) o) : vec(v), offset(o) {}
    vector_view(const vector_view<T>& v, const decltype(offset) o) : vec(v.vec), offset(v.offset + o) {}
};

struct Operand {};

enum class AccessSpecifier {
    DIRECT,
    REGISTER_INDIRECT,
    POINTER_DEREFERENCE,
};

enum class RegisterSetSpecifier {
    CURRENT,
    LOCAL,
    STATIC,
    GLOBAL,
};

struct RegisterIndex : public Operand {
    AccessSpecifier as;
    viua::internals::types::register_index index;
    RegisterSetSpecifier rss;
};
struct InstructionBlockName : public Operand {};
struct BitsLiteral : public Operand {
    string content;
};
struct IntegerLiteral : public Operand {
    string content;
};
struct FloatLiteral : public Operand {
    string content;
};
struct BooleanLiteral : public Operand {
    string content;
};
struct VoidLiteral : public Operand {
    const string content = "void";
};
struct FunctionNameLiteral : public Operand {
    string content;
};
struct AtomLiteral : public Operand {
    string content;
};
struct TextLiteral : public Operand {
    string content;
};

struct Instruction {
    OPCODE opcode;
    vector<unique_ptr<Operand>> operands;
};

struct InstructionsBlock {
    string name;
    map<string, string> attributes;
    vector<Instruction> body;
};

struct ParsedSource {
    vector<InstructionsBlock> functions;
    vector<InstructionsBlock> blocks;
};


static auto parse_attribute_value(const vector_view<Token> tokens, string& value) -> const
    decltype(tokens)::size_type {
    auto i = decltype(tokens)::size_type{1};

    if (tokens.at(i) == ")") {
        // do nothing
    } else {
        value = tokens.at(i).str();
        ++i;
    }

    if (tokens.at(i) != ")") {
        throw viua::cg::lex::InvalidSyntax(tokens.at(i), "expected ')'");
    }

    return i;
}
static auto parse_attributes(const vector_view<Token> tokens,
                             decltype(InstructionsBlock::attributes) & attributes) -> const
    decltype(tokens)::size_type {
    auto i = decltype(tokens)::size_type{0};

    if (tokens.at(i) != "[[") {
        throw InvalidSyntax(tokens.at(i));
    }
    ++i;  // skip '[['

    while (i < tokens.size() and tokens.at(i) != "]]") {
        const string key = tokens.at(i++);
        string value;

        if (tokens.at(i) == ",") {
            ++i;
        } else if (tokens.at(i) == "(") {
            i += parse_attribute_value(vector_view<Token>(tokens, i), value);
        } else if (tokens.at(i) == "]]") {
            // do nothing
        } else {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), "expected ',' or '(' or ']]'");
        }

        cerr << "  attribute: " << key << '(' << value << ')' << endl;
        attributes[key] = value;
    }
    ++i;  // skip ']]'

    if (i == tokens.size()) {
        throw InvalidSyntax(tokens.at(i - 1), "unexpected end-of-file: expected function name");
    }

    return i;
}

static auto parse_operand(const vector_view<Token> tokens, unique_ptr<Operand>& operand) -> const
    decltype(tokens)::size_type {
    auto i = std::remove_reference_t<decltype(tokens)>::size_type{0};

    auto tok = tokens.at(i).str();

    if (tok.at(0) == '%' or tok.at(0) == '*' or tok.at(0) == '@') {
        auto ri = make_unique<RegisterIndex>();

        if (tok.at(0) == '%') {
            ri->as = AccessSpecifier::DIRECT;
        } else if (tok.at(0) == '*') {
            ri->as = AccessSpecifier::POINTER_DEREFERENCE;
        } else if (tok.at(0) == '@') {
            ri->as = AccessSpecifier::REGISTER_INDIRECT;
        }

        ri->index = static_cast<decltype(ri->index)>(stoul(tok.substr(1)));
        ++i;

        if (tokens.at(i) == "current") {
            ri->rss = RegisterSetSpecifier::CURRENT;
        } else if (tokens.at(i) == "local") {
            ri->rss = RegisterSetSpecifier::LOCAL;
        } else if (tokens.at(i) == "static") {
            ri->rss = RegisterSetSpecifier::STATIC;
        } else if (tokens.at(i) == "global") {
            ri->rss = RegisterSetSpecifier::GLOBAL;
        }
        ++i;

        operand = std::move(ri);
    } else if (str::is_binary_literal(tok)) {
        auto bits_literal = make_unique<BitsLiteral>();
        bits_literal->content = tokens.at(i);
        ++i;

        operand = std::move(bits_literal);
    } else if (str::isnum(tok, true)) {
        auto integer_literal = make_unique<IntegerLiteral>();
        integer_literal->content = tokens.at(i);
        ++i;

        operand = std::move(integer_literal);
    } else if (str::isfloat(tok, true)) {
        auto float_literal = make_unique<FloatLiteral>();
        float_literal->content = tokens.at(i);
        ++i;

        operand = std::move(float_literal);
    } else if (str::is_boolean_literal(tok)) {
        auto boolean_literal = make_unique<BooleanLiteral>();
        boolean_literal->content = tokens.at(i);
        ++i;

        operand = std::move(boolean_literal);
    } else if (str::is_void(tok)) {
        auto void_literal = make_unique<VoidLiteral>();
        ++i;

        operand = std::move(void_literal);
    } else if (assembler::utils::isValidFunctionName(tok)) {
        auto fn_name_literal = make_unique<FunctionNameLiteral>();
        fn_name_literal->content = tokens.at(i);
        ++i;

        operand = std::move(fn_name_literal);
    } else if (str::is_atom_literal(tok)) {
        auto atom_literal = make_unique<AtomLiteral>();
        atom_literal->content = tokens.at(i);
        ++i;

        operand = std::move(atom_literal);
    } else if (str::is_text_literal(tok)) {
        auto text_literal = make_unique<TextLiteral>();
        text_literal->content = tokens.at(i);
        ++i;

        operand = std::move(text_literal);
    } else {
        throw viua::cg::lex::InvalidSyntax(tokens.at(i), "invalid operand");
    }

    return i;
}

static auto mnemonic_to_opcode(const string mnemonic) -> OPCODE {
    OPCODE opcode = NOP;
    for (const auto each : OP_NAMES) {
        if (each.second == mnemonic) {
            opcode = each.first;
            break;
        }
    }
    return opcode;
}
static auto parse_instruction(const vector_view<Token> tokens, Instruction& instruction)
    -> decltype(tokens)::size_type {
    auto i = std::remove_reference_t<decltype(tokens)>::size_type{0};

    if (not OP_MNEMONICS.count(tokens.at(i).str())) {
        throw viua::cg::lex::InvalidSyntax(tokens.at(i), "expected mnemonic");
    }

    cerr << "  mnemonic: " << tokens.at(i).str() << endl;
    instruction.opcode = mnemonic_to_opcode(tokens.at(i++).str());

    while (tokens.at(i) != "\n") {
        unique_ptr<Operand> operand;
        i += parse_operand(vector_view<Token>(tokens, i), operand);
    }
    ++i;  // skip newline

    return i;
}

static auto parse_block_body(const vector_view<Token> tokens, decltype(InstructionsBlock::body) & body)
    -> decltype(tokens)::size_type {
    auto i = std::remove_reference_t<decltype(tokens)>::size_type{0};

    while (tokens.at(i) != ".end") {
        Instruction instruction;
        i += parse_instruction(vector_view<Token>(tokens, i), instruction);
        body.push_back(std::move(instruction));
    }

    return i;
}

static auto parse_function(const vector_view<Token> tokens, InstructionsBlock& ib) -> const
    decltype(tokens)::size_type {
    auto i = std::remove_reference_t<decltype(tokens)>::size_type{1};

    cerr << "parsing function" << endl;

    i += parse_attributes(vector_view<Token>(tokens, i), ib.attributes);

    cerr << "  name: " << tokens.at(i).str() << endl;
    ib.name = tokens.at(i).str();
    ++i;  // skip name
    ++i;  // skip newline

    i += parse_block_body(vector_view<Token>(tokens, i), ib.body);

    return i;
}

static auto parse(const vector<Token>& tokens) -> ParsedSource {
    ParsedSource parsed;

    for (auto i = std::remove_reference_t<decltype(tokens)>::size_type{0}; i < tokens.size(); ++i) {
        if (tokens.at(i) == "\n") {
            continue;
        }
        if (tokens.at(i) == ".function:") {
            InstructionsBlock ib;
            i += parse_function(vector_view<Token>(tokens, i), ib);
        } else {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), "expected '.function:' or newline");
        }
    }

    return parsed;
}


static auto display_result(const ParsedSource&) -> void {}


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
    auto tokens = viua::cg::lex::cook(raw_tokens);

    vector<Token> normalised_tokens = normalise(tokens);

    try {
        ParsedSource parsed_source = parse(normalised_tokens);
        display_result(parsed_source);
    } catch (const viua::cg::lex::InvalidSyntax& e) {
        display_error_in_context(raw_tokens, e, filename);
        return 1;
    } catch (const viua::cg::lex::TracedSyntaxError& e) {
        display_error_in_context(raw_tokens, e, filename);
        return 1;
    }

    return 0;
}
