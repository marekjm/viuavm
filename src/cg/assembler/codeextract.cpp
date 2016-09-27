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

#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <set>
#include <viua/bytecode/maps.h>
#include <viua/support/string.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/cg/lex.h>
#include <viua/program.h>
using namespace std;


using Token = viua::cg::lex::Token;


static void assert_is_not_reserved_keyword(Token token, const string& message) {
    string s = token;
    static const set<string> reserved_keywords {
        /*
         * Used for timeouts in 'join' and 'receive' instructions
         */
        "infinity",

        /*
         *  Reserved as register set names.
         */
        "local",
        "static",
        "global",

        /*
         * Reserved for future use.
         */
        "auto",
        "default",
        "undefined",
        "null",
        "void",
        "iota",

        /*
         * Reserved for future use as boolean literals.
         */
        "true",
        "false",

        /*
         * Reserved for future use as instruction names.
         */
        "int",
        "int8",
        "int16",
        "int32",
        "int64",
        "uint",
        "uint8",
        "uint16",
        "uint32",
        "uint64",
        "float32",
        "float64",
        "string",
        "bits",
    };
    if (reserved_keywords.count(s) or OP_SIZES.count(s)) {
        throw viua::cg::lex::InvalidSyntax(token, ("invalid " + message + ": '" + s + "' is a registered keyword"));
    }
}


vector<string> assembler::ce::getilines(const vector<string>& lines) {
    /*  Clears code from empty lines and comments.
     */
    vector<string> ilines;
    string line;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (!line.size() or line[0] == ';' or str::startswith(line, "--")) continue;
        ilines.emplace_back(line);
    }

    return ilines;
}

map<string, int> assembler::ce::getmarks(const vector<viua::cg::lex::Token>& tokens) {
    /** This function will pass over all instructions and
     * gather "marks", i.e. `.mark: <name>` directives which may be used by
     * `jump` and `branch` instructions.
     */
    map<string, int> marks;
    int instruction = 0;  // we need separate instruction counter because number of lines is not exactly number of instructions

    for (decltype(tokens.size()) i = 0; i < tokens.size(); ++i) {
        if (tokens.at(i) == ".name:" or tokens.at(i) == ".link:") {
            do {
                ++i;
            } while (i < tokens.size() and tokens.at(i) != "\n");
            continue;
        }
        if (tokens.at(i) == ".mark:") {
            ++i;    // skip ".mark:" token
            assert_is_not_reserved_keyword(tokens.at(i), "marker name");
            marks.emplace(tokens.at(i), instruction);
            ++i;    // skip marker name
            continue;
        }
        if (tokens.at(i) == "\n") {
            ++instruction;
        }
    }

    return marks;
}

map<string, int> assembler::ce::getnames(const vector<viua::cg::lex::Token>& tokens) {
    /** This function will pass over all instructions and
     *  gather "names", i.e. `.name: <register> <name>` instructions which may be used by
     *  as substitutes for register indexes to more easily remember what is stored where.
     *
     *  Example name instruction: `.name: 1 base`.
     *  This allows to access first register with name `base` instead of its index.
     *
     *  Example (which also uses marks) name reference could be: `branch if_equals_0 :finish`.
     */
    map<string, int> names;
    string reg, name;
    for (decltype(tokens.size()) i = 0; i < tokens.size(); ++i) {
        if (tokens.at(i) == ".name:") {
            reg = tokens.at(i+1);
            name = tokens.at(i+2);
            assert_is_not_reserved_keyword(tokens.at(i+2), "register name");
        } else {
            continue;
        }

        try {
            names[name] = stoi(reg);
        } catch (const std::invalid_argument& e) {
            throw ("invalid register index in name directive: " + str::strencode(name) + " := " + str::enquote(str::strencode(reg)));
        }
    }
    return names;
}

vector<string> assembler::ce::getlinks(const vector<viua::cg::lex::Token>& tokens) {
    /** This function will pass over all instructions and
     * gather .link: assembler instructions.
     */
    vector<string> links;
    for (decltype(tokens.size()) i = 0; i < tokens.size(); ++i) {
        if (tokens.at(i) == ".link:") {
            ++i;    // skip '.link:' token
            if (tokens.at(i) == "\n") {
                throw viua::cg::lex::InvalidSyntax(tokens.at(i), "missing module name in link directive");
            }
            links.emplace_back(tokens.at(i));
            ++i;    // skip module name token
        }
    }
    return links;
}

static vector<string> get_instruction_block_names(const vector<Token>& tokens, string directive) {
    vector<string> names;

    const auto limit = tokens.size();
    string looking_for = ("." + directive + ":");
    for (decltype(tokens.size()) i = 0; i < limit; ++i) {
        if (tokens[i].str() == looking_for) {
            ++i;
            if (i < limit) {
                names.emplace_back(tokens.at(i).str());
            } else {
                throw tokens[i-1];
            }
        }
    }

    return names;
}
vector<string> assembler::ce::getFunctionNames(const vector<Token>& tokens) {
    auto names = get_instruction_block_names(tokens, "function");
    for (const auto& each : get_instruction_block_names(tokens, "closure")) {
        names.push_back(each);
    }
    return names;
}
vector<string> assembler::ce::getSignatures(const vector<Token>& tokens) {
    return get_instruction_block_names(tokens, "signature");
}
vector<string> assembler::ce::getBlockNames(const vector<Token>& tokens) {
    return get_instruction_block_names(tokens, "block");
}
vector<string> assembler::ce::getBlockSignatures(const vector<Token>& tokens) {
    return get_instruction_block_names(tokens, "bsignature");
}


static map<string, vector<Token>> get_raw_block_bodies(const string& type, const vector<Token>& tokens) {
    map<string, vector<Token>> invokables;

    string looking_for = ("." + type + ":");
    string name;
    vector<Token> body;

    for (decltype(tokens.size()) i = 0; i < tokens.size(); ++i) {
        if (tokens[i] == looking_for) {
            ++i; // skip directive
            name = tokens[i];
            ++i; // skip name
            ++i; // skip '\n' token
            while (i < tokens.size() and tokens[i].str() != ".end") {
                if (tokens[i] == looking_for) {
                    throw viua::cg::lex::InvalidSyntax(tokens[i], ("another " + type + " opened before assembler reached .end after '" + name + "' " + type));
                }
                body.push_back(tokens[i]);
                ++i;
            }
            ++i; // skip .end token

            invokables[name] = body;
            name = "";
            body.clear();
        }
    }

    return invokables;
}
static vector<string> make_lines(const vector<Token>& tokens) {
    vector<string> lines;

    if (tokens.size() == 0) {
        return lines;
    }

    auto current_line_no = tokens.at(0).line();
    ostringstream current_line;
    for (const auto& token : tokens) {
        if ((token.line() != current_line_no) or (token.str() == "\n")) {
            string line = current_line.str();
            if (line.size() > 0) {
                lines.emplace_back(current_line.str());
            }
            current_line_no = token.line();
            current_line.str("");
        }
        if (token.str() == "\n") {
            continue;
        }
        current_line << token.str() << ' ';
    }
    if (current_line.str().size()) {
        lines.emplace_back(current_line.str());
    }

    return lines;
}
static map<string, vector<string>> get_cooked_block_bodies(map<string, vector<Token>> raw_bodies) {
    map<string, vector<string>> cooked_bodies;

    for (const auto& each : raw_bodies) {
        cooked_bodies[each.first] = make_lines(each.second);
    }

    return cooked_bodies;
}
map<string, vector<string>> assembler::ce::getInvokables(const string& type, const vector<Token>& tokens) {
    return get_cooked_block_bodies(get_raw_block_bodies(type, tokens));
}
map<string, vector<Token>> assembler::ce::getInvokablesTokenBodies(const string& type, const vector<Token>& tokens) {
    return get_raw_block_bodies(type, tokens);
}
