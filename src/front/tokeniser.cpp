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
#include <utility>
#include <functional>
#include <viua/support/string.h>
#include <viua/support/env.h>
#include <viua/version.h>
using namespace std;


// MISC FLAGS
bool SHOW_HELP = false;
bool SHOW_VERSION = false;
bool VERBOSE = false;


class Token {
    string content;
    decltype(content.size()) line_number, character_in_line;

    public:

    auto line() const -> decltype(line_number) {
        return line_number;
    }
    auto character() const -> decltype(character_in_line) {
        return character_in_line;
    }
    string str() const {
        return content;
    }

    auto ends() const -> decltype(character_in_line) {
        return (character_in_line + content.size());
    }

    bool operator==(const string& s) {
        return (content == s);
    }
    bool operator!=(const string& s) {
        return (content != s);
    }

    Token(decltype(line_number) line_, decltype(character_in_line) character_, string content_): content(content_), line_number(line_), character_in_line(character_) {
    }
};

struct InvalidSyntax {
    unsigned line_number, character_in_line;
    string content;

    InvalidSyntax(unsigned ln, unsigned ch, string ct): line_number(ln), character_in_line(ch), content(ct) {
    }
};

static vector<Token> tokenise(const string& source) {
    vector<Token> tokens;

    ostringstream candidate_token;
    candidate_token.str("");

    decltype(candidate_token.str().size()) line_number = 0, character_in_line = 0;

    const auto limit = source.size();
    for (decltype(source.size()) i = 0; i < limit; ++i) {
        char current_char = source[i];
        bool found_breaking_character = false;

        switch (current_char) {
            case '\n':
            case '\t':
            case ' ':
            case '~':
            case '`':
            case '!':
            case '@':
            case '#':
            case '$':
            case '%':
            case '^':
            case '&':
            case '*':
            case '(':
            case ')':
            case '-':
            case '+':
            case '=':
            case '{':
            case '}':
            case '[':
            case ']':
            case '|':
            case ':':
            case ';':
            case ',':
            case '.':
            case '<':
            case '>':
            case '/':
            case '?':
            case '\'':
            case '"':
                found_breaking_character = true;
                break;
            default:
                candidate_token << source[i];
        }

        if (found_breaking_character) {
            if (candidate_token.str().size()) {
                tokens.emplace_back(line_number, character_in_line, candidate_token.str());
                character_in_line += candidate_token.str().size();
                candidate_token.str("");
            }
            if (current_char == '\'' or current_char == '"') {
                string s = str::extract(source.substr(i));
                tokens.emplace_back(line_number, character_in_line, s);
                character_in_line += (s.size() - 1);
                i += (s.size() - 1);
            } else {
                candidate_token << current_char;
                tokens.emplace_back(line_number, character_in_line, candidate_token.str());
                candidate_token.str("");
            }

            ++character_in_line;
            if (current_char == '\n') {
                ++line_number;
                character_in_line = 0;
            }
        }
    }

    return tokens;
}

template<class T, typename... R> bool adjacent(T first, T second) {
    if (first.line() != second.line()) {
        return false;
    }
    if (first.ends() != second.character()) {
        return false;
    }
    return true;
}
template<class T, typename... R> bool adjacent(T first, T second, R... rest) {
    if (first.line() != second.line()) {
        return false;
    }
    if (first.ends() != second.character()) {
        return false;
    }
    return adjacent(second, rest...);
}

static string join_tokens(const vector<Token> tokens, const decltype(tokens)::size_type from, const decltype(from) to) {
    ostringstream joined;

    for (auto i = from; i < tokens.size() and i < to; ++i) {
        joined << tokens[i].str();
    }

    return joined.str();
}

static vector<Token> remove_spaces(vector<Token> input_tokens) {
    vector<Token> tokens;

    string space(" ");
    for (auto t : input_tokens) {
        if (t.str() != space) {
            tokens.push_back(t);
        }
    }

    return tokens;
}

static vector<Token> remove_comments(vector<Token> input_tokens) {
    vector<Token> tokens;

    string colon(";");
    string newline("\n");
    for (auto it = input_tokens.begin(); it < input_tokens.end(); ++it) {
        if (it->str() != colon) {
            tokens.push_back(*it);
        } else {
            do {
                ++it;
            } while (it->str() != newline);
            tokens.push_back(*it);
        }
    }

    return tokens;
}

static vector<Token> reduce_newlines(vector<Token> input_tokens) {
    vector<Token> tokens;

    for (auto it = input_tokens.begin(); it < input_tokens.end(); ++it) {
        if (*it == "\n") {
            tokens.push_back(*it);
            while (*it == "\n") {
                ++it;
            }
            --it;
        } else {
            tokens.push_back(*it);
        }
    }

    return tokens;
}

static vector<Token> reduce_function_directive(vector<Token> input_tokens) {
    decltype(input_tokens) tokens;

    const auto limit = input_tokens.size();
    for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
        const auto t = input_tokens[i];
        if (t.str() == "." and i < limit-2 and input_tokens[i+1] == "function" and input_tokens[i+2].str() == ":") {
            if (adjacent(t, input_tokens[i+1], input_tokens[i+2])) {
                tokens.emplace_back(t.line(), t.character(), ".function:");
                ++i; // skip "function" token
                ++i; // skip ":" token
                continue;
            }
        }
        tokens.push_back(t);
    }

    return tokens;
}

static vector<Token> reduce_end_directive(vector<Token> input_tokens) {
    decltype(input_tokens) tokens;

    const auto limit = input_tokens.size();
    for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
        const auto t = input_tokens[i];
        if (t.str() == "." and i < limit-1 and input_tokens[i+1] == "end") {
            if (adjacent(t, input_tokens[i+1])) {
                tokens.emplace_back(t.line(), t.character(), ".end");
                ++i; // skip "end" token
                continue;
            }
        }
        tokens.push_back(t);
    }

    return tokens;
}

static vector<Token> reduce_double_colon(vector<Token> input_tokens) {
    decltype(input_tokens) tokens;

    const auto limit = input_tokens.size();
    for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
        const auto t = input_tokens[i];
        if (t.str() == ":" and i < limit-1 and input_tokens[i+1].str() == ":") {
            if (adjacent(t, input_tokens[i+1])) {
                tokens.emplace_back(t.line(), t.character(), "::");
                ++i; // skip second ":" token
                continue;
            }
        }
        tokens.push_back(t);
    }

    return tokens;
}

static vector<Token> reduce_function_signatures(vector<Token> input_tokens) {
    decltype(input_tokens) tokens;

    const auto limit = input_tokens.size();
    for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
        const auto t = input_tokens[i];
        if (str::isid(t.str())) {
            auto j = i;
            while (j+2 < limit and input_tokens[j+1].str() == "::" and str::isid(input_tokens[j+2].str()) and adjacent(input_tokens[j], input_tokens[j+1], input_tokens[j+2])) {
                ++j; // swallow "::" token
                ++j; // swallow identifier token
            }
            if (j+1 < limit and input_tokens[j+1].str() == "/") {
                if (j+2 < limit and str::isnum(input_tokens[j+2].str(), false)) {
                    if (adjacent(input_tokens[j], input_tokens[j+1], input_tokens[j+2])) {
                        tokens.emplace_back(t.line(), t.character(), join_tokens(input_tokens, i, j+3));
                        ++j; // skip "/" token
                        ++j; // skip integer encoding arity
                        i = j;
                        continue;
                    }
                } else if (j+2 < limit and input_tokens[j+2].str() == "\n") {
                    if (adjacent(input_tokens[j], input_tokens[j+1])) {
                        tokens.emplace_back(t.line(), t.character(), join_tokens(input_tokens, i, j+2));
                        ++j; // skip "/" token
                        i = j;
                        continue;
                    }
                }
            }
        }
        tokens.push_back(t);
    }

    return tokens;
}

static vector<Token> unwrap_lines(vector<Token> input_tokens, bool full = true) {
    decltype(input_tokens) unwrapped_tokens;
    decltype(input_tokens) tokens;
    decltype(input_tokens) final_tokens;

    decltype(tokens.size()) i = 0, limit = input_tokens.size();
    bool invert = false;
    while (i < limit) {
        Token t = input_tokens[i];
        if (t == "^") {
            invert = true;
            ++i;
            continue;
        }
        if (t == "(") {
            vector<Token> subtokens;
            ++i;
            unsigned balance = 1;
            while (i < limit) {
                if (input_tokens[i] == "(") {
                    ++balance;
                } else if (input_tokens[i] == ")") {
                    --balance;
                }
                if (balance == 0) {
                    ++i;
                    break;
                }
                subtokens.push_back(input_tokens[i]);
                ++i;
            }

            Token tok = subtokens.at(1);

            for (auto subt : unwrap_lines(subtokens)) {
                unwrapped_tokens.push_back(subt);
            }
            unwrapped_tokens.emplace_back(t.line(), t.character(), "\n");

            if (invert) {
                final_tokens.push_back(tok);
                while (i < limit and input_tokens[i] != "\n") {
                    final_tokens.push_back(input_tokens[i]);
                    ++i;
                }

                // this line is to push \n to the token list
                final_tokens.push_back(input_tokens[i++]);

                for (auto to : unwrapped_tokens) {
                    final_tokens.push_back(to);
                }
            } else {
                vector<Token> foo;
                while (final_tokens.size() and final_tokens[final_tokens.size()-1] != "\n") {
                    foo.insert(foo.begin(), final_tokens.back());
                    final_tokens.pop_back();
                }

                for (auto to : unwrapped_tokens) {
                    final_tokens.push_back(to);
                }

                for (auto to : foo) {
                    final_tokens.push_back(to);
                }
                if (full) {
                    final_tokens.push_back(tok);
                }
            }

            invert = false;
            unwrapped_tokens.clear();
            tokens.clear();

            continue;
        }
        if (t == "[") {
            vector<Token> subtokens;
            ++i;
            unsigned balance = 1;
            unsigned toplevel_subexpressions_balance = 0;
            unsigned toplevel_subexpressions = 0;
            while (i < limit) {
                if (input_tokens[i] == "[") {
                    ++balance;
                } else if (input_tokens[i] == "]") {
                    --balance;
                }
                if (input_tokens[i] == "(") {
                    if ((++toplevel_subexpressions_balance) == 1) { ++toplevel_subexpressions; }
                }
                if (input_tokens[i] == ")") {
                    --toplevel_subexpressions_balance;
                }
                if (balance == 0) {
                    ++i;
                    break;
                }
                subtokens.push_back(input_tokens[i]);
                ++i;
            }

            Token tok(subtokens.at(0).line(), subtokens.at(0).character(), str::stringify(toplevel_subexpressions));

            subtokens = unwrap_lines(subtokens, false);

            for (auto subt : unwrap_lines(subtokens)) {
                unwrapped_tokens.push_back(subt);
            }
            unwrapped_tokens.emplace_back(t.line(), t.character(), "\n");

            if (invert) {
                final_tokens.push_back(tok);
                while (i < limit and input_tokens[i] != "\n") {
                    final_tokens.push_back(input_tokens[i]);
                    ++i;
                }

                // this line is to push \n to the token list
                final_tokens.push_back(input_tokens[i++]);

                for (auto to : unwrapped_tokens) {
                    final_tokens.push_back(to);
                }
            } else {
                vector<Token> foo;
                while (final_tokens.size() and final_tokens[final_tokens.size()-1] != "\n") {
                    foo.insert(foo.begin(), final_tokens.back());
                    final_tokens.pop_back();
                }

                for (auto to : unwrapped_tokens) {
                    final_tokens.push_back(to);
                }

                for (auto to : foo) {
                    final_tokens.push_back(to);
                }
                final_tokens.push_back(tok);
            }

            invert = false;
            unwrapped_tokens.clear();
            tokens.clear();

            continue;
        }
        final_tokens.push_back(t);
        ++i;
    }

    return final_tokens;
}

template<class T> static auto enumerate(const vector<T>& v) -> vector<pair<typename vector<T>::size_type, T>> {
    vector<pair<typename vector<T>::size_type, T>> enumerated_vector;

    typename vector<T>::size_type i = 0;
    for (const auto& each : v) {
        enumerated_vector.emplace_back(i, each);
        ++i;
    }

    return enumerated_vector;
}

static void encode_json(const string& filename, const vector<Token>& tokens) {
    const string INDENT = "  ";
    cout << "{\n";
    cout << INDENT << str::enquote("file") << ": " << str::enquote(filename) << ",\n";
    cout << INDENT << str::enquote("tokens") << ": [\n";

    const auto limit = tokens.size();
    for (const auto& t : enumerate(tokens)) {
        cout << INDENT << INDENT << "{\n";
        cout << INDENT << INDENT << INDENT << str::enquote("line") << ": " << t.second.line() << ",\n";
        cout << INDENT << INDENT << INDENT << str::enquote("character") << ": " << t.second.character() << ",\n";
        cout << INDENT << INDENT << INDENT << str::enquote("content") << ": " << str::enquote(str::strencode(t.second.str())) << "\n";
        cout << INDENT << INDENT << '}';
        if (t.first+1 < limit) {
            cout << ',';
        }
        cout << '\n';
    }

    cout << INDENT << "]\n";
    cout << "}\n";
}

static bool usage(const char* program, bool show_help, bool show_version, bool verbose) {
    if (show_help or (show_version and verbose)) {
        cout << "Viua VM tokenizer, version ";
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
        } else if (str::startswith(option, "-")) {
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

    ostringstream source_in;
    string line;
    while (getline(in, line)) { source_in << line << '\n'; }

    string source = source_in.str();

    vector<Token> tokens;
    try {
        tokens = reduce_function_signatures(reduce_double_colon(reduce_end_directive(reduce_function_directive(unwrap_lines(reduce_newlines(remove_comments(remove_spaces(tokenise(source)))))))));
        encode_json(filename, tokens);
    } catch (const InvalidSyntax& e) {
        cerr << filename << ':' << e.line_number << ':' << e.character_in_line << ": error: invalid syntax: " << e.content << endl;
        return 1;
    }

    return 0;
}
