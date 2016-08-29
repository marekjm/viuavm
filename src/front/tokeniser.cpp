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

    Token(unsigned line_, unsigned character_, string content_): content(content_), line_number(line_), character_in_line(character_) {
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

    string newline("\n");
    for (auto it = input_tokens.begin(); it < input_tokens.end(); ++it) {
        if (it->str() == newline) {
            tokens.push_back(*it);
            while (it->str() == newline) {
                ++it;
            }
            --it;
        } else {
            tokens.push_back(*it);
        }
    }

    return tokens;
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
        tokens = reduce_newlines(remove_comments(remove_spaces(tokenise(source))));
        cout << tokens.size() << endl;

        for (const auto& t : tokens) {
            cout << t.line() << ':' << t.character() << ".." << t.ends() << ": ```" << t.str() << "'''" << endl;
        }
    } catch (const InvalidSyntax& e) {
        cerr << filename << ':' << e.line_number << ':' << e.character_in_line << ": error: invalid syntax: " << e.content << endl;
        return 1;
    }

    return 0;
}
