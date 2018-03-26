/*
 *  Copyright (C) 2016, 2017, 2018 Marek Marecki
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

#include <map>
#include <set>
#include <sstream>
#include <viua/bytecode/maps.h>
#include <viua/cg/lex.h>
#include <viua/support/string.h>
using namespace std;

namespace viua {
    namespace cg {
        namespace lex {
            auto Token::line() const -> decltype(line_number) { return line_number; }
            auto Token::character() const -> decltype(character_in_line) { return character_in_line; }

            auto Token::str() const -> decltype(content) { return content; }
            auto Token::str(string s) -> void { content = s; }

            auto Token::original() const -> decltype(original_content) { return original_content; }
            auto Token::original(string s) -> void { original_content = s; }

            auto Token::ends(bool const as_original) const -> decltype(character_in_line) {
                return (character_in_line + (as_original ? original_content : content).size());
            }

            auto Token::operator==(string const& s) const -> bool { return (content == s); }
            auto Token::operator!=(string const& s) const -> bool { return (content != s); }

            Token::operator string() const { return str(); }

            Token::Token(decltype(line_number) line_, decltype(character_in_line) character_, string content_)
                : content(content_),
                  original_content(content),
                  line_number(line_),
                  character_in_line(character_) {}
            Token::Token() : Token(0, 0, "") {}

            auto InvalidSyntax::what() const -> const char* { return message.c_str(); }
            auto InvalidSyntax::str() const -> string { return message; }

            auto InvalidSyntax::line() const -> decltype(line_number) { return line_number; }
            auto InvalidSyntax::character() const -> decltype(character_in_line) { return character_in_line; }

            auto InvalidSyntax::match(Token const token) const -> bool {
                for (const auto& each : tokens) {
                    if (token.line() == each.line() and token.character() == each.character()) {
                        return true;
                    }
                    if (token.line() != each.line()) {
                        continue;
                    }
                    if (token.character() >= each.character() and token.ends(true) <= each.ends(true)) {
                        return true;
                    }
                }
                return false;
            }

            auto InvalidSyntax::add(Token token) -> InvalidSyntax& {
                tokens.push_back(std::move(token));
                return *this;
            }

            auto InvalidSyntax::note(string n) -> InvalidSyntax& {
                attached_notes.push_back(n);
                return *this;
            }
            auto InvalidSyntax::notes() const -> const decltype(attached_notes)& { return attached_notes; }

            auto InvalidSyntax::aside(string a) -> InvalidSyntax& {
                aside_note = a;
                return *this;
            }
            auto InvalidSyntax::aside(Token t, string a) -> InvalidSyntax& {
                aside_note = a;
                aside_token = t;
                return *this;
            }
            auto InvalidSyntax::aside() const -> string { return aside_note; }

            auto InvalidSyntax::match_aside(Token token) const -> bool {
                if (token.line() == aside_token.line() and token.character() == aside_token.character()) {
                    return true;
                }
                if (token.line() != aside_token.line()) {
                    return false;
                }
                if (token.character() >= aside_token.character() and
                    token.ends(true) <= aside_token.ends(true)) {
                    return true;
                }
                return false;
            }

            InvalidSyntax::InvalidSyntax(decltype(line_number) ln, decltype(character_in_line) ch, string ct)
                : line_number(ln), character_in_line(ch), content(ct) {}
            InvalidSyntax::InvalidSyntax(Token t, string m)
                : line_number(t.line()), character_in_line(t.character()), content(t.original()), message(m) {
                add(t);
            }

            UnusedValue::UnusedValue(Token token)
                : InvalidSyntax(token, ("unused value in register " + token.str())) {}

            UnusedValue::UnusedValue(Token token, string s) : InvalidSyntax(token, s) {}

            auto TracedSyntaxError::what() const -> const char* { return errors.front().what(); }

            auto TracedSyntaxError::line() const -> decltype(errors.front().line()) {
                return errors.front().line();
            }
            auto TracedSyntaxError::character() const -> decltype(errors.front().character()) {
                return errors.front().character();
            }

            auto TracedSyntaxError::append(InvalidSyntax const& e) -> TracedSyntaxError& {
                errors.push_back(e);
                return (*this);
            }


            auto is_mnemonic(string const& s) -> bool {
                auto is_it = false;
                for (const auto& each : OP_NAMES) {
                    if (each.second == s) {
                        is_it = true;
                        break;
                    }
                }
                return is_it;
            }
            auto is_reserved_keyword(string const& s) -> bool {
                static set<string> const reserved_keywords{
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
                    "const",

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
                    "boolean",
                    "coroutine",
                    "yield",
                    "channel",
                    "publish",
                    "subscribe",
                };
                return (reserved_keywords.count(s) or is_mnemonic(s));
            }
            auto assert_is_not_reserved_keyword(Token token, string const& message) -> void {
                auto const s = token.original();
                if (is_reserved_keyword(s)) {
                    throw viua::cg::lex::InvalidSyntax(
                        token, ("invalid " + message + ": '" + s + "' is a registered keyword"));
                }
            }

            auto tokenise(string const& source) -> vector<Token> {
                vector<Token> tokens;

                ostringstream candidate_token;
                candidate_token.str("");

                decltype(candidate_token.str().size()) line_number = 0, character_in_line = 0;

                const auto limit = source.size();

                unsigned hyphens = 0;
                bool active_comment = false;
                for (decltype(source.size()) i = 0; i < limit; ++i) {
                    char current_char = source.at(i);
                    bool found_breaking_character = false;

                    switch (current_char) {
                        case '\n':
                        case '-':
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
                            candidate_token << source.at(i);
                    }

                    if (current_char == ';') {
                        active_comment = true;
                    }
                    if (current_char == '-') {
                        if (++hyphens >= 2) {
                            active_comment = true;
                        }
                    } else {
                        hyphens = 0;
                    }

                    if (found_breaking_character) {
                        if (candidate_token.str().size()) {
                            tokens.emplace_back(line_number, character_in_line, candidate_token.str());
                            character_in_line += candidate_token.str().size();
                            candidate_token.str("");
                        }
                        if ((current_char == '\'' or current_char == '"') and not active_comment) {
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
                            active_comment = false;
                        }
                    }
                }

                return tokens;
            }

            static auto is_register_set_name(string const s) -> bool {
                return (s == "current" or s == "local" or s == "static" or s == "global");
            }
            static auto is_register_index(string const s) -> bool {
                auto const p = s.at(0);
                return (p == '%' or p == '*' or p == '@');
            }
            auto standardise(vector<Token> input_tokens) -> vector<Token> {
                vector<Token> tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    Token token = input_tokens.at(i);
                    if (token == "call" or token == "process" or token == "msg") {
                        tokens.push_back(token);
                        if (is_register_index(input_tokens.at(i + 1)) or (input_tokens.at(i + 1) == "void")) {
                            tokens.push_back(input_tokens.at(++i));
                        } else {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "void");
                        }
                        if (is_register_index(tokens.back())) {
                            if (is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.push_back(input_tokens.at(++i));
                            } else {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    "current");
                            }
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (is_register_index(tokens.back())) {
                            if (is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.push_back(input_tokens.at(++i));
                            } else {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    "current");
                            }
                        }

                        tokens.push_back(input_tokens.at(++i));
                    } else if (token == "tailcall" or token == "defer") {
                        tokens.push_back(token);

                        tokens.push_back(input_tokens.at(++i));
                        if (is_register_index(tokens.back())) {
                            if (is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.push_back(input_tokens.at(++i));
                            } else {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    "current");
                            }
                        }

                        tokens.push_back(input_tokens.at(++i));
                    } else if (token == "frame") {
                        tokens.push_back(token);

                        if ((not str::isnum(input_tokens.at(i + 1).str(), false)) and
                            input_tokens.at(i + 1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i + 1).line(),
                                                input_tokens.at(i + 1).character(), "%0");
                            tokens.emplace_back(input_tokens.at(i + 1).line(),
                                                input_tokens.at(i + 1).character(), "%16");
                            continue;
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if ((not str::isnum(input_tokens.at(i + 1).str(), false)) and
                            input_tokens.at(i + 1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i + 1).line(),
                                                input_tokens.at(i + 1).character(), "%16");
                        }
                    } else if (token == "param" or token == "pamv") {
                        tokens.push_back(token);  // mnemonic

                        tokens.push_back(input_tokens.at(++i));  // target register

                        tokens.push_back(input_tokens.at(++i));  // source register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "arg") {
                        tokens.push_back(token);  // mnemonic

                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (tokens.back() != "void") {
                            if (not is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    "current");
                            } else {
                                tokens.push_back(input_tokens.at(++i));
                            }
                        }

                        tokens.push_back(input_tokens.at(++i));
                    } else if (token == "vector") {
                        tokens.push_back(token);
                        tokens.push_back(input_tokens.at(++i));

                        string target_register_index = tokens.back();
                        string target_register_set = "current";
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                            tokens.back().original(target_register_index);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            target_register_set = tokens.back();
                        }

                        if (input_tokens.at(i + 1) == "\n") {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "%0");
                            tokens.back().original("\n");

                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                            tokens.back().original("\n");

                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "%0");
                            tokens.back().original("\n");
                            continue;
                        }

                        // pack start register
                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                            tokens.back().original("\n");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        if ((not str::isnum(input_tokens.at(i + 1).str(), false)) and
                            input_tokens.at(i + 1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i + 1).line(),
                                                input_tokens.at(i + 1).character(),
                                                "%0");  // number of registers to pack
                            tokens.back().original("\n");
                        }
                    } else if (token == "vpop") {
                        tokens.push_back(token);

                        tokens.push_back(input_tokens.at(++i));
                        string target_register_set = "current";
                        if (tokens.back().str() != "void") {
                            if (not is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    target_register_set);
                            } else {
                                tokens.push_back(input_tokens.at(++i));
                                target_register_set = tokens.back().str();
                            }
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        if (input_tokens.at(i + 1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i + 1).line(),
                                                input_tokens.at(i + 1).character(), "void");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            if (not is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    target_register_set);
                            } else {
                                tokens.push_back(input_tokens.at(++i));
                            }
                        }
                    } else if (token == "vat") {
                        tokens.push_back(token);

                        tokens.push_back(input_tokens.at(++i));
                        string target_register_set = "current";
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            target_register_set = tokens.back().str();
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "vlen") {
                        tokens.push_back(token);

                        tokens.push_back(input_tokens.at(++i));
                        string target_register_set = "current";
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            target_register_set = tokens.back();
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "vinsert") {
                        tokens.push_back(token);

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        if (input_tokens.at(i + 1).str() == "\n") {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "void");
                        }
                    } else if (token == "vpush") {
                        tokens.push_back(token);

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "insert" or token == "structinsert") {
                        tokens.push_back(token);  // mnemonic

                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));  // source register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));  // key register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "remove" or token == "structremove") {
                        tokens.push_back(token);  // mnemonic

                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (tokens.back() != "void") {
                            if (not is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    "current");
                            } else {
                                tokens.push_back(input_tokens.at(++i));
                            }
                        }

                        tokens.push_back(input_tokens.at(++i));  // source register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));  // key register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "join") {
                        tokens.push_back(token);

                        tokens.push_back(input_tokens.at(++i));
                        if (tokens.back() != "void") {
                            if (is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.push_back(input_tokens.at(++i));
                            } else {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    "current");
                            }
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.push_back(input_tokens.at(++i));
                        } else {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        }

                        if (input_tokens.at(i + 1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i + 1).line(),
                                                input_tokens.at(i + 1).character(), "infinity");
                        }
                    } else if (token == "receive") {
                        tokens.push_back(token);
                        tokens.push_back(input_tokens.at(++i));

                        if (tokens.back() != "void") {
                            if (is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.push_back(input_tokens.at(++i));
                            } else {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    "current");
                            }
                        }

                        if (input_tokens.at(i + 1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i + 1).line(),
                                                input_tokens.at(i + 1).character(),
                                                "infinity");  // number of registers to pack
                        }
                    } else if (token == "add" or token == "sub" or token == "mul" or token == "div" or
                               token == "lt" or token == "lte" or token == "gt" or token == "gte" or
                               token == "eq") {
                        tokens.push_back(token);  // mnemonic

                        tokens.push_back(input_tokens.at(++i));  // target register
                        string target_register_index = tokens.back();
                        string target_register_set = "current";
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            target_register_set = tokens.back();
                        }

                        // lhs register
                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        if (input_tokens.at(i + 1) == "\n") {
                            auto popped_target_rs = tokens.back();
                            tokens.pop_back();
                            auto popped_target_ri = tokens.back();
                            tokens.pop_back();

                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_index);
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);

                            tokens.push_back(popped_target_ri);
                            tokens.push_back(popped_target_rs);
                            continue;
                        }

                        // rhs register
                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "and" or token == "or") {
                        tokens.push_back(token);  // mnemonic

                        // target operand
                        tokens.push_back(input_tokens.at(++i));
                        if (is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.push_back(input_tokens.at(++i));
                        } else {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        }

                        // lhs operand
                        tokens.push_back(input_tokens.at(++i));
                        if (is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.push_back(input_tokens.at(++i));
                        } else {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        }

                        // rhs operand
                        tokens.push_back(input_tokens.at(++i));
                        if (is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.push_back(input_tokens.at(++i));
                        } else {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        }
                    } else if (token == "capture" or token == "capturecopy" or token == "capturemove") {
                        tokens.push_back(token);  // mnemonic

                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));  // source register

                        if (input_tokens.at(i + 1).str() == "\n") {
                            // if only two operands are given, double source register
                            // this will expand the following instruction:
                            //
                            //      opcode T S
                            // to:
                            //
                            //      opcode T S S
                            //
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                tokens.back().str());
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                            continue;
                        } else {
                            tokens.push_back(input_tokens.at(++i));  // source register
                        }
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "closure" or token == "function") {
                        tokens.push_back(token);                 // mnemonic
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "current");
                        }
                    } else if (token == "integer") {
                        tokens.push_back(token);                 // mnemonic
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "current");
                        }
                        if (input_tokens.at(i + 1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "0");
                        }
                        continue;
                    } else if (token == "float") {
                        tokens.push_back(token);                 // mnemonic
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "current");
                        }
                        if (input_tokens.at(i + 1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "0.0");
                        }
                        continue;
                    } else if (token == "string") {
                        tokens.push_back(token);                 // mnemonic
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "current");
                        }
                        if (input_tokens.at(i + 1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "\"\"");
                        }
                        continue;
                    } else if (token == "text") {
                        tokens.push_back(token);                 // mnemonic
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "current");
                        }
                        if (input_tokens.at(i + 1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "\"\"");
                            continue;
                        }
                        if (is_register_index(input_tokens.at(i + 1))) {
                            /*
                             *  This form of the "text" instruction is used to obtain text representations of
                             * Viua values.
                             *  Its syntax is:
                             *
                             *          text {output:r-operand} {input:r-operand}
                             *
                             *  instead of:
                             *
                             *          text {output:r-operand} "{text literal}"
                             */
                            tokens.push_back(input_tokens.at(++i));
                            if (not is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    "current");
                            }
                            continue;
                        }
                    } else if (token == "texteq" or token == "atomeq") {
                        tokens.push_back(token);  // mnemonic

                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));  // lhs register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));  // rhs register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "atom") {
                        tokens.push_back(token);                 // mnemonic
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "current");
                        }
                        continue;
                    } else if (token == "itof" or token == "ftoi" or token == "stoi" or token == "stof") {
                        tokens.push_back(token);  // mnemonic

                        tokens.push_back(input_tokens.at(++i));  // target register
                        // save target register index because we may need to insert it later
                        auto target_index = tokens.back();

                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        // save target register set because we may need to insert it later
                        auto target_rs = tokens.back();

                        // source register
                        if (input_tokens.at(i + 1).str() == "\n") {
                            // copy target register index
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                target_index);
                            // copy target register set
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                target_rs);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            if (not is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                    target_rs);
                            }
                        }
                        continue;
                    } else if (token == "if") {
                        tokens.push_back(token);  // mnemonic
                        if (input_tokens.at(i + 1) == "\n") {
                            throw viua::cg::lex::InvalidSyntax(token, "branch without operands");
                        }

                        tokens.push_back(input_tokens.at(++i));  // checked register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        if (input_tokens.at(i + 1) == "\n") {
                            throw viua::cg::lex::InvalidSyntax(token, "branch without a target");
                        }

                        tokens.push_back(input_tokens.at(++i));  // target if true branch
                        if (input_tokens.at(i + 1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "+1");
                        }
                        continue;
                    } else if (token == "not") {
                        tokens.push_back(token);  // mnemonic

                        tokens.push_back(input_tokens.at(++i));  // target register
                        string target_register_set = "current";
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            target_register_set = tokens.back();
                        }

                        if (input_tokens.at(i + 1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                input_tokens.at(i).str());
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                            continue;
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "move" or token == "copy" or token == "swap" or token == "ptr" or
                               token == "isnull" or token == "send" or token == "textlength" or
                               token == "structkeys" or token == "bits" or token == "bitset" or
                               token == "bitat") {
                        tokens.push_back(token);  // mnemonic

                        if (input_tokens.at(i + 1) == "[[") {
                            do {
                                tokens.push_back(input_tokens.at(++i));
                            } while (input_tokens.at(i) != "]]");
                        }

                        tokens.push_back(input_tokens.at(++i));  // target register
                        string target_register_set = "current";
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            target_register_set = tokens.back();
                        }

                        if (input_tokens.at(i + 1) == "\n") {
                            throw InvalidSyntax{input_tokens.at(i + 1), "missing second operand"}.add(token);
                        }
                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "class" or token == "derive" or token == "attach" or
                               token == "register" or token == "new") {
                        tokens.push_back(token);                 // mnemonic
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                            tokens.back().original(input_tokens.at(i));
                        }
                    } else if (token == "izero" or token == "print" or token == "argc" or token == "echo" or
                               token == "delete" or token == "draw" or token == "throw" or token == "iinc" or
                               token == "idec" or token == "self" or token == "struct") {
                        tokens.push_back(token);                 // mnemonic
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (input_tokens.at(i + 1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i + 1).line(),
                                                input_tokens.at(i + 1).character(), "current");
                            tokens.back().original(input_tokens.at(i + 1));
                        }
                        continue;
                    } else {
                        tokens.push_back(token);
                    }
                }

                return tokens;
            }
            auto normalise(vector<Token> input_tokens) -> vector<Token> {
                vector<Token> tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    Token token = input_tokens.at(i);

                    /*
                     * Leading token is always a mnemonic or a directive.
                     *
                     * FIXME This is not *always* a leading token because there are "holes" in normalise
                     * function: instructions or directives which either do not need, or do not have a special
                     * treatment implemented.
                     */
                    tokens.push_back(token);

                    if (token == "call" or token == "process" or token == "msg") {
                        if (is_register_index(input_tokens.at(i + 1)) or (input_tokens.at(i + 1) == "void")) {
                            tokens.push_back(input_tokens.at(++i));
                        } else {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "void");
                        }
                        if (is_register_index(tokens.back())) {
                            if (is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.push_back(input_tokens.at(++i));
                            } else {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    "current");
                            }
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (is_register_index(tokens.back())) {
                            if (is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.push_back(input_tokens.at(++i));
                            } else {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    "current");
                            }
                        }

                        tokens.push_back(input_tokens.at(++i));
                    } else if (token == "tailcall" or token == "defer") {
                        tokens.push_back(input_tokens.at(++i));
                        if (is_register_index(tokens.back())) {
                            if (is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.push_back(input_tokens.at(++i));
                            } else {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    "current");
                            }
                        }

                        tokens.push_back(input_tokens.at(++i));
                    } else if (token == "frame") {
                        if ((not str::isnum(input_tokens.at(i + 1).str(), false)) and
                            input_tokens.at(i + 1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i + 1).line(),
                                                input_tokens.at(i + 1).character(), "%0");
                            tokens.emplace_back(input_tokens.at(i + 1).line(),
                                                input_tokens.at(i + 1).character(), "%16");
                            continue;
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if ((not str::isnum(input_tokens.at(i + 1).str(), false)) and
                            input_tokens.at(i + 1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i + 1).line(),
                                                input_tokens.at(i + 1).character(), "%16");
                        }
                    } else if (token == "param" or token == "pamv") {
                        tokens.push_back(input_tokens.at(++i));  // target register

                        tokens.push_back(input_tokens.at(++i));  // source register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "arg") {
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (tokens.back() != "void") {
                            if (not is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    "current");
                            } else {
                                tokens.push_back(input_tokens.at(++i));
                            }
                        }

                        tokens.push_back(input_tokens.at(++i));
                    } else if (token == "vector") {
                        tokens.push_back(input_tokens.at(++i));

                        string target_register_index = tokens.back();
                        string target_register_set = "current";
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                            tokens.back().original(target_register_index);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            target_register_set = tokens.back();
                        }

                        if (input_tokens.at(i + 1) == "\n") {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "%0");
                            tokens.back().original("\n");

                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                            tokens.back().original("\n");

                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "%0");
                            tokens.back().original("\n");
                            continue;
                        }

                        // pack start register
                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                            tokens.back().original("\n");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        if ((not str::isnum(input_tokens.at(i + 1).str(), false)) and
                            input_tokens.at(i + 1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i + 1).line(),
                                                input_tokens.at(i + 1).character(),
                                                "%0");  // number of registers to pack
                            tokens.back().original("\n");
                        }
                    } else if (token == "vpop") {
                        tokens.push_back(input_tokens.at(++i));
                        string target_register_set = "current";
                        if (tokens.back().str() != "void") {
                            if (not is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    target_register_set);
                            } else {
                                tokens.push_back(input_tokens.at(++i));
                                target_register_set = tokens.back().str();
                            }
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        if (input_tokens.at(i + 1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i + 1).line(),
                                                input_tokens.at(i + 1).character(), "void");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            if (not is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    target_register_set);
                            } else {
                                tokens.push_back(input_tokens.at(++i));
                            }
                        }
                    } else if (token == "vat") {
                        tokens.push_back(input_tokens.at(++i));
                        string target_register_set = "current";
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            target_register_set = tokens.back().str();
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "vlen") {
                        tokens.push_back(input_tokens.at(++i));
                        string target_register_set = "current";
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            target_register_set = tokens.back();
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "vinsert") {
                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        if (input_tokens.at(i + 1).str() == "\n") {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "%0");
                        }
                    } else if (token == "vpush") {
                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "insert" or token == "structinsert") {
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));  // source register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));  // key register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "remove" or token == "structremove") {
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (tokens.back() != "void") {
                            if (not is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    "current");
                            } else {
                                tokens.push_back(input_tokens.at(++i));
                            }
                        }

                        tokens.push_back(input_tokens.at(++i));  // source register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));  // key register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "join") {
                        tokens.push_back(input_tokens.at(++i));
                        if (tokens.back() != "void") {
                            if (is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.push_back(input_tokens.at(++i));
                            } else {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    "current");
                            }
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.push_back(input_tokens.at(++i));
                        } else {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        }

                        if (input_tokens.at(i + 1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i + 1).line(),
                                                input_tokens.at(i + 1).character(), "infinity");
                        }
                    } else if (token == "receive") {
                        tokens.push_back(input_tokens.at(++i));

                        if (tokens.back() != "void") {
                            if (is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.push_back(input_tokens.at(++i));
                            } else {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    "current");
                            }
                        }

                        if (input_tokens.at(i + 1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i + 1).line(),
                                                input_tokens.at(i + 1).character(),
                                                "infinity");  // number of registers to pack
                        }
                    } else if (token == "add" or token == "sub" or token == "mul" or token == "div" or
                               token == "lt" or token == "lte" or token == "gt" or token == "gte" or
                               token == "eq" or token == "wrapadd" or token == "wrapmul") {
                        tokens.push_back(input_tokens.at(++i));  // target register
                        string target_register_index = tokens.back();
                        string target_register_set = "current";
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            target_register_set = tokens.back();
                        }

                        // lhs register
                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        if (input_tokens.at(i + 1) == "\n") {
                            auto popped_target_rs = tokens.back();
                            tokens.pop_back();
                            auto popped_target_ri = tokens.back();
                            tokens.pop_back();

                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_index);
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);

                            tokens.push_back(popped_target_ri);
                            tokens.push_back(popped_target_rs);
                            continue;
                        }

                        // rhs register
                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "and" or token == "or") {
                        // target operand
                        tokens.push_back(input_tokens.at(++i));
                        if (is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.push_back(input_tokens.at(++i));
                        } else {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        }

                        // lhs operand
                        tokens.push_back(input_tokens.at(++i));
                        if (is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.push_back(input_tokens.at(++i));
                        } else {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        }

                        // rhs operand
                        tokens.push_back(input_tokens.at(++i));
                        if (is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.push_back(input_tokens.at(++i));
                        } else {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        }
                    } else if (token == "capture" or token == "capturecopy" or token == "capturemove") {
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));  // source register

                        if (input_tokens.at(i + 1).str() == "\n") {
                            // if only two operands are given, double source register
                            // this will expand the following instruction:
                            //
                            //      opcode T S
                            // to:
                            //
                            //      opcode T S S
                            //
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                tokens.back().str());
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                            continue;
                        } else {
                            tokens.push_back(input_tokens.at(++i));  // source register
                        }
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "closure" or token == "function") {
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "current");
                        }
                    } else if (token == "integer") {
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "current");
                        }
                        if (input_tokens.at(i + 1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "0");
                        }
                        continue;
                    } else if (token == "float") {
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "current");
                        }
                        if (input_tokens.at(i + 1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "0.0");
                        }
                        continue;
                    } else if (token == "string") {
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "current");
                        }
                        if (input_tokens.at(i + 1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "\"\"");
                        }
                        continue;
                    } else if (token == "text") {
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "current");
                        }
                        if (input_tokens.at(i + 1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "\"\"");
                            continue;
                        }
                        if (is_register_index(input_tokens.at(i + 1))) {
                            /*
                             *  This form of the "text" instruction is used to obtain text representations of
                             * Viua values.
                             *  Its syntax is:
                             *
                             *          text {output:r-operand} {input:r-operand}
                             *
                             *  instead of:
                             *
                             *          text {output:r-operand} "{text literal}"
                             */
                            tokens.push_back(input_tokens.at(++i));
                            if (not is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                    "current");
                            }
                            continue;
                        }
                    } else if (token == "texteq" or token == "atomeq") {
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));  // lhs register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));  // rhs register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "atom") {
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "current");
                        }
                        continue;
                    } else if (token == "itof" or token == "ftoi" or token == "stoi" or token == "stof") {
                        tokens.push_back(input_tokens.at(++i));  // target register
                        // save target register index because we may need to insert it later
                        auto target_index = tokens.back();

                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        // save target register set because we may need to insert it later
                        auto target_rs = tokens.back();

                        // source register
                        if (input_tokens.at(i + 1).str() == "\n") {
                            // copy target register index
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                target_index);
                            // copy target register set
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                target_rs);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            if (not is_register_set_name(input_tokens.at(i + 1))) {
                                tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                    target_rs);
                            }
                        }
                        continue;
                    } else if (token == "if") {
                        if (input_tokens.at(i + 1) == "\n") {
                            throw viua::cg::lex::InvalidSyntax(token, "branch without operands");
                        }

                        tokens.push_back(input_tokens.at(++i));  // checked register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        if (input_tokens.at(i + 1) == "\n") {
                            throw viua::cg::lex::InvalidSyntax(token, "branch without a target");
                        }

                        tokens.push_back(input_tokens.at(++i));  // target if true branch
                        if (input_tokens.at(i + 1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                "+1");
                        }
                        continue;
                    } else if (token == "not") {
                        tokens.push_back(input_tokens.at(++i));  // target register
                        string target_register_set = "current";
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            target_register_set = tokens.back();
                        }

                        if (input_tokens.at(i + 1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(),
                                                input_tokens.at(i).str());
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                            continue;
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "move" or token == "copy" or token == "swap" or token == "ptr" or
                               token == "isnull" or token == "send" or token == "textlength" or
                               token == "structkeys" or token == "bitset" or token == "bitat") {
                        if (input_tokens.at(i + 1) == "[[") {  // FIXME attributes
                            do {
                                tokens.push_back(input_tokens.at(++i));
                            } while (input_tokens.at(i) != "]]");
                        }

                        tokens.push_back(input_tokens.at(++i));  // target register
                        string target_register_set = "current";
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            target_register_set = tokens.back();
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "bits") {
                        tokens.push_back(input_tokens.at(++i));  // target register
                        string target_register_set = "current";
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            target_register_set = tokens.back();
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (str::is_binary_literal(tokens.back())) {
                            continue;
                        }
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(),
                                                target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "class" or token == "derive" or token == "attach" or
                               token == "register" or token == "new") {
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (not is_register_set_name(input_tokens.at(i + 1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                            tokens.back().original(input_tokens.at(i));
                        }
                    } else if (token == "izero" or token == "print" or token == "argc" or token == "echo" or
                               token == "delete" or token == "draw" or token == "throw" or token == "iinc" or
                               token == "idec" or token == "self" or token == "struct" or
                               token == "wrapincrement" or token == "wrapdecrement") {
                        tokens.push_back(input_tokens.at(++i));  // target register
                        if (input_tokens.at(i + 1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i + 1).line(),
                                                input_tokens.at(i + 1).character(), "current");
                            tokens.back().original(input_tokens.at(i + 1));
                        }
                        continue;
                    } else if (token == ".function:" or token == ".closure:" or token == ".block:") {
                        if (input_tokens.at(i + 1) != "[[") {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "[[");
                            tokens.back().original(input_tokens.at(i));
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "]]");
                            tokens.back().original(input_tokens.at(i));
                        }
                        continue;
                    } else {
                        // do nothing
                    }
                }

                return tokens;
            }

            auto join_tokens(vector<Token> const tokens, decltype(tokens)::size_type const from,
                             decltype(from) const to) -> string {
                ostringstream joined;

                for (auto i = from; i < tokens.size() and i < to; ++i) {
                    joined << tokens.at(i).str();
                }

                return joined.str();
            }
        }  // namespace lex
    }      // namespace cg
}  // namespace viua
