#include <sstream>
#include <map>
#include <set>
#include <viua/bytecode/maps.h>
#include <viua/support/string.h>
#include <viua/cg/lex.h>
using namespace std;

namespace viua {
    namespace cg {
        namespace lex {
            auto Token::line() const -> decltype(line_number) {
                return line_number;
            }
            auto Token::character() const -> decltype(character_in_line) {
                return character_in_line;
            }

            auto Token::str() const -> decltype(content) {
                return content;
            }
            auto Token::str(string s) -> void {
                content = s;
            }

            auto Token::original() const -> decltype(original_content) {
                return original_content;
            }
            auto Token::original(string s) -> void {
                original_content = s;
            }

            auto Token::ends() const -> decltype(character_in_line) {
                return (character_in_line + content.size());
            }

            bool Token::operator==(const string& s) const {
                return (content == s);
            }
            bool Token::operator!=(const string& s) const {
                return (content != s);
            }

            Token::operator string() const {
                return str();
            }

            Token::Token(decltype(line_number) line_, decltype(character_in_line) character_, string content_): content(content_), original_content(content), line_number(line_), character_in_line(character_) {
            }
            Token::Token(): Token(0, 0, "") {}

            const char* InvalidSyntax::what() const {
                return message.c_str();
            }

            auto InvalidSyntax::line() const -> decltype(line_number) {
                return line_number;
            }
            auto InvalidSyntax::character() const -> decltype(character_in_line) {
                return character_in_line;
            }

            auto InvalidSyntax::match(Token token) const -> bool {
                for (const auto& each : tokens) {
                    if (token.line() == each.line() and token.character() == each.character()) {
                        return true;
                    }
                    if (token.line() == each.line() and token.character() >= each.character() and token.ends() <= each.ends()) {
                        return true;
                    }
                }
                return false;
            }

            auto InvalidSyntax::add(Token token) -> InvalidSyntax& {
                tokens.push_back(std::move(token));
                return *this;
            }

            InvalidSyntax::InvalidSyntax(decltype(line_number) ln, decltype(character_in_line) ch, string ct): line_number(ln), character_in_line(ch), content(ct) {
            }
            InvalidSyntax::InvalidSyntax(Token t, string m): line_number(t.line()), character_in_line(t.character()), content(t.original()), message(m) {
                add(t);
            }

            UnusedValue::UnusedValue(Token token): InvalidSyntax(token, ("unused value in register " + token.str())) {
            }

            const char* TracedSyntaxError::what() const {
                return errors.front().what();
            }

            auto TracedSyntaxError::line() const -> decltype(errors.front().line()) {
                return errors.front().line();
            }
            auto TracedSyntaxError::character() const -> decltype(errors.front().character()) {
                return errors.front().character();
            }

            auto TracedSyntaxError::append(const InvalidSyntax& e) -> TracedSyntaxError& {
                errors.push_back(e);
                return (*this);
            }


            bool is_reserved_keyword(const string& s) {
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
                    "bits",
                    "coroutine",
                    "yield",
                    "channel",
                    "publish",
                    "subscribe",

                    /*
                     * Reserved  for future use as bit-operation instruction names.
                     * Shifts:
                     *
                     *      shl <target> <source> <width>
                     *
                     *          logical bit shift left;
                     *          <source> is shifted left by <width> bits
                     *          bits shifted out of <source> are put in <target>
                     *          <target> has the same bitsize as <source>
                     *
                     *      ashr <target> <source> <width>
                     *
                     *          arithmetic bit shift right;
                     *          same as logical bit shift right, only the highest bit is preserved
                     *
                     *      ashl <target> <source> <width>
                     *
                     *          arithmetic bit shift left;
                     *          same as logical bit shift left, only the lowest bit is preserved
                     */
                    "shl",  // logical shift left
                    "shr",  // logical shift right
                    "ashl", // arithmetic shift left
                    "ashr", // arithmetic shift right

                    "rol",  // rotate left
                    "ror",  // rotate right
                };
                return (reserved_keywords.count(s) or OP_MNEMONICS.count(s));
            }
            void assert_is_not_reserved_keyword(Token token, const string& message) {
                string s = token.original();
                if (is_reserved_keyword(s)) {
                    throw viua::cg::lex::InvalidSyntax(token, ("invalid " + message + ": '" + s + "' is a registered keyword"));
                }
            }

            vector<Token> tokenise(const string& source) {
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

            static auto is_register_set_name(string s) -> bool {
                return (s == "current" or s == "local" or s == "static" or s == "global");
            }
            static auto is_register_index(string s) -> bool {
                const auto p = s.at(0);
                return (p == '%' or p == '*' or p == '@');
            }
            vector<Token> standardise(vector<Token> input_tokens) {
                vector<Token> tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    Token token = input_tokens.at(i);
                    if (token == "call" or token == "process") {
                        tokens.push_back(token);
                        if (is_register_index(input_tokens.at(i+1)) or (input_tokens.at(i+1) == "void")) {
                            tokens.push_back(input_tokens.at(++i));
                        } else {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "void");
                        }
                        if (is_register_index(tokens.back())) {
                            if (is_register_set_name(input_tokens.at(i+1))) {
                                tokens.push_back(input_tokens.at(++i));
                            } else {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                            }
                        }
                        tokens.push_back(input_tokens.at(++i));
                    } else if (token == "fcall") {
                        tokens.push_back(token);

                        tokens.push_back(input_tokens.at(++i));
                        if (tokens.back().str() != "void") {
                            if (not is_register_set_name(input_tokens.at(i+1))) {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                            } else {
                                tokens.push_back(input_tokens.at(++i));
                            }
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "frame") {
                        tokens.push_back(token);

                        if ((not str::isnum(input_tokens.at(i+1).str(), false)) and input_tokens.at(i+1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "%0");
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "%16");
                            continue;
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if ((not str::isnum(input_tokens.at(i+1).str(), false)) and input_tokens.at(i+1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "%16");
                        }
                    } else if (token == "vec") {
                        tokens.push_back(token);
                        tokens.push_back(input_tokens.at(++i));

                        string target_register_index = tokens.back();
                        string target_register_set = "current";
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), target_register_set);
                            tokens.back().original(target_register_index);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            target_register_set = tokens.back();
                        }

                        if (input_tokens.at(i+1) == "\n") {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "%0");
                            tokens.back().original("\n");

                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), target_register_set);
                            tokens.back().original("\n");

                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "%0");
                            tokens.back().original("\n");
                            continue;
                        }

                        // pack start register
                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), target_register_set);
                            tokens.back().original("\n");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        if ((not str::isnum(input_tokens.at(i+1).str(), false)) and input_tokens.at(i+1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "%0"); // number of registers to pack
                            tokens.back().original("\n");
                        }
                    } else if (token == "vpop") {
                        tokens.push_back(token);

                        tokens.push_back(input_tokens.at(++i));
                        if (tokens.back().str() != "void") {
                            if (not is_register_set_name(input_tokens.at(i+1))) {
                                tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                            } else {
                                tokens.push_back(input_tokens.at(++i));
                            }
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        if (input_tokens.at(i+1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "-1");
                        }
                    } else if (token == "vat") {
                        tokens.push_back(token);

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        if (input_tokens.at(i+1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "-1");
                        }
                    } else if (token == "vlen") {
                        tokens.push_back(token);

                        tokens.push_back(input_tokens.at(++i));
                        string target_register_set = "current";
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            target_register_set = tokens.back();
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "vinsert") {
                        tokens.push_back(token);

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        if (input_tokens.at(i+1).str() == "\n") {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "%0");
                        }
                    } else if (token == "vpush") {
                        tokens.push_back(token);

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "join") {
                        tokens.push_back(token);
                        tokens.push_back(input_tokens.at(++i));
                        tokens.push_back(input_tokens.at(++i));

                        if (input_tokens.at(i+1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "infinity"); // number of registers to pack
                        }
                    } else if (token == "receive") {
                        tokens.push_back(token);
                        tokens.push_back(input_tokens.at(++i));

                        if (input_tokens.at(i+1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "infinity"); // number of registers to pack
                        }
                    } else if (token == "add" or token == "sub" or token == "mul" or token == "div" or
                               token == "lt" or token == "lte" or token == "gt" or token == "gte" or token == "eq"
                    ) {
                        tokens.push_back(token);    // mnemonic
                        tokens.push_back(input_tokens.at(++i)); // result type specifier

                        tokens.push_back(input_tokens.at(++i)); // target register

                        string target_register_index = tokens.back();
                        string target_register_set = "current";
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            target_register_set = tokens.back();
                        }

                        // lhs register
                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        if (input_tokens.at(i+1) == "\n") {
                            auto popped_target_rs = tokens.back();
                            tokens.pop_back();
                            auto popped_target_ri = tokens.back();
                            tokens.pop_back();

                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), target_register_index);
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), target_register_set);

                            tokens.push_back(popped_target_ri);
                            tokens.push_back(popped_target_rs);
                            continue;
                        }

                        // rhs register
                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "and" or token == "or") {
                        tokens.push_back(token);    // mnemonic
                        tokens.push_back(input_tokens.at(++i)); // target register
                        if (input_tokens.at(i+2).str() == "\n") {
                            // if only two operands are given, double target register
                            // this will expand the following instruction:
                            //
                            //      opcode T S
                            // to:
                            //
                            //      opcode T T S
                            //
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), tokens.back().str());
                            continue;
                        }
                        tokens.push_back(input_tokens.at(++i)); // second source register
                    } else if (token == "capture" or token == "capturecopy" or token == "capturemove") {
                        tokens.push_back(token);    // mnemonic

                        tokens.push_back(input_tokens.at(++i)); // target register
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        tokens.push_back(input_tokens.at(++i)); // source register

                        if (input_tokens.at(i+1).str() == "\n") {
                            // if only two operands are given, double source register
                            // this will expand the following instruction:
                            //
                            //      opcode T S
                            // to:
                            //
                            //      opcode T S S
                            //
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), tokens.back().str());
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                            continue;
                        } else {
                            tokens.push_back(input_tokens.at(++i)); // source register
                        }
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "closure" or token == "function") {
                        tokens.push_back(token);                // mnemonic
                        tokens.push_back(input_tokens.at(++i)); // target register
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "current");
                        }
                    } else if (token == "istore") {
                        tokens.push_back(token);                // mnemonic
                        tokens.push_back(input_tokens.at(++i)); // target register
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "current");
                        }
                        if (input_tokens.at(i+1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "0");
                        }
                        continue;
                    } else if (token == "fstore") {
                        tokens.push_back(token);                // mnemonic
                        tokens.push_back(input_tokens.at(++i)); // target register
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "current");
                        }
                        if (input_tokens.at(i+1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "0.0");
                        }
                        continue;
                    } else if (token == "strstore") {
                        tokens.push_back(token);                // mnemonic
                        tokens.push_back(input_tokens.at(++i)); // target register
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "current");
                        }
                        if (input_tokens.at(i+1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "\"\"");
                        }
                        continue;
                    } else if (token == "itof" or token == "ftoi" or token == "stoi" or token == "stof") {
                        tokens.push_back(token);                // mnemonic

                        tokens.push_back(input_tokens.at(++i)); // target register
                        // save target register index because we may need to insert it later
                        auto target_index = tokens.back();

                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "current");
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }

                        // save target register set because we may need to insert it later
                        auto target_rs = tokens.back();

                        // source register
                        if (input_tokens.at(i+1).str() == "\n") {
                            // copy target register index
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), target_index);
                            // copy target register set
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), target_rs);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            if (not is_register_set_name(input_tokens.at(i+1))) {
                                tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), target_rs);
                            }
                        }
                        continue;
                    } else if (token == "if") {
                        tokens.push_back(token);                // mnemonic
                        if (input_tokens.at(i+1) == "\n") {
                            throw viua::cg::lex::InvalidSyntax(token, "branch without operands");
                        }

                        tokens.push_back(input_tokens.at(++i)); // checked register
                        if (input_tokens.at(i+1) == "\n") {
                            throw viua::cg::lex::InvalidSyntax(token, "branch without a target");
                        }

                        tokens.push_back(input_tokens.at(++i)); // target if true branch
                        if (input_tokens.at(i+1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "+1");
                        }
                        continue;
                    } else if (token == "not") {
                        tokens.push_back(token);                // mnemonic

                        tokens.push_back(input_tokens.at(++i)); // target register
                        string target_register_set = "current";
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            target_register_set = tokens.back();
                        }

                        if (input_tokens.at(i+1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), input_tokens.at(i).str());
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), target_register_set);
                            continue;
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "move" or token == "copy" or token == "swap" or token == "ptr" or token == "isnull") {
                        tokens.push_back(token);                // mnemonic

                        tokens.push_back(input_tokens.at(++i)); // target register
                        string target_register_set = "current";
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                            target_register_set = tokens.back();
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if (not is_register_set_name(input_tokens.at(i+1))) {
                            tokens.emplace_back(tokens.back().line(), tokens.back().character(), target_register_set);
                        } else {
                            tokens.push_back(input_tokens.at(++i));
                        }
                    } else if (token == "izero"
                        or token == "print"
                        or token == "echo"
                        or token == "delete"
                        or token == "throw"
                        or token == "iinc"
                        or token == "idec"
                    ) {
                        tokens.push_back(token);                // mnemonic
                        tokens.push_back(input_tokens.at(++i)); // target register
                        if (input_tokens.at(i+1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "current");
                            tokens.back().original(input_tokens.at(i+1));
                        }
                        continue;
                    } else {
                        tokens.push_back(token);
                    }
                }

                return tokens;
            }

            string join_tokens(const vector<Token> tokens, const decltype(tokens)::size_type from, const decltype(from) to) {
                ostringstream joined;

                for (auto i = from; i < tokens.size() and i < to; ++i) {
                    joined << tokens.at(i).str();
                }

                return joined.str();
            }

            vector<Token> remove_spaces(vector<Token> input_tokens) {
                vector<Token> tokens;

                string space(" ");
                for (auto t : input_tokens) {
                    if (t.str() != space) {
                        tokens.push_back(t);
                    }
                }

                return tokens;
            }

            vector<Token> remove_comments(vector<Token> input_tokens) {
                vector<Token> tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    Token token = input_tokens.at(i);
                    if (token.str() == ";") {
                        // FIXME this is ugly as hell
                        do {
                            ++i;
                        } while (i < limit and input_tokens.at(i).str() != "\n");
                        if (i < limit) {
                            tokens.push_back(input_tokens.at(i));
                        }
                    } else if (i+2 < limit and adjacent(token, input_tokens.at(i+1)) and token.str() == "-" and input_tokens.at(i+1).str() == "-") {
                        // FIXME this is ugly as hell
                        do {
                            ++i;
                        } while (i < limit and input_tokens.at(i).str() != "\n");
                        if (i < limit) {
                            tokens.push_back(input_tokens.at(i));
                        }
                    } else {
                        tokens.push_back(token);
                    }
                }

                return tokens;
            }

            vector<Token> reduce_newlines(vector<Token> input_tokens) {
                vector<Token> tokens;

                const auto limit = input_tokens.size();
                decltype(input_tokens)::size_type i = 0;
                while (i < limit and input_tokens.at(i) == "\n") {
                    ++i;
                }
                for (; i < limit; ++i) {
                    Token token = input_tokens.at(i);
                    if (token.str() == "\n") {
                        tokens.push_back(token);
                        while (i < limit and input_tokens.at(i).str() == "\n") {
                            ++i;
                        }
                        --i;
                    } else {
                        tokens.push_back(token);
                    }
                }

                return tokens;
            }

            static auto is_valid_register_id(string s) -> bool {
                return (str::isnum(s) or str::isid(s));
            }
            static bool match(const vector<Token>& tokens, std::remove_reference<decltype(tokens)>::type::size_type i, const vector<string>& sequence) {
                if (i+sequence.size() >= tokens.size()) {
                    return false;
                }

                decltype(i) n = 0;
                while (n < sequence.size()) {
                    // empty string in the sequence means "any token"
                    if ((not sequence.at(n).empty()) and tokens.at(i+n) != sequence.at(n)) {
                        return false;
                    }
                    ++n;
                }

                return true;
            }

            static bool match_adjacent(const vector<Token>& tokens, std::remove_reference<decltype(tokens)>::type::size_type i, const vector<string>& sequence) {
                if (i+sequence.size() >= tokens.size()) {
                    return false;
                }

                decltype(i) n = 0;
                while (n < sequence.size()) {
                    // empty string in the sequence means "any token"
                    if ((not sequence.at(n).empty()) and tokens.at(i+n) != sequence.at(n)) {
                        return false;
                    }
                    if (n and not adjacent(tokens.at(i+n-1), tokens.at(i+n))) {
                        return false;
                    }
                    ++n;
                }

                return true;
            }

            static vector<Token> reduce_token_sequence(vector<Token> input_tokens, vector<string> sequence) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);
                    if (match_adjacent(input_tokens, i, sequence)) {
                        tokens.emplace_back(t.line(), t.character(), join_tokens(input_tokens, i, (i+sequence.size())));
                        i += (sequence.size()-1);
                        continue;
                    }
                    tokens.push_back(t);
                }

                return tokens;
            }

            static vector<Token> reduce_directive(vector<Token> input_tokens, string directive) {
                return reduce_token_sequence(input_tokens, {".", directive, ":"});
            }

            vector<Token> reduce_mark_directive(vector<Token> input_tokens) {
                return reduce_directive(input_tokens, "mark");
            }

            vector<Token> reduce_name_directive(vector<Token> input_tokens) {
                return reduce_directive(input_tokens, "name");
            }

            vector<Token> reduce_info_directive(vector<Token> input_tokens) {
                return reduce_directive(input_tokens, "info");
            }

            vector<Token> reduce_main_directive(vector<Token> input_tokens) {
                return reduce_directive(input_tokens, "main");
            }

            vector<Token> reduce_link_directive(vector<Token> input_tokens) {
                return reduce_directive(input_tokens, "link");
            }

            vector<Token> reduce_function_directive(vector<Token> input_tokens) {
                return reduce_directive(input_tokens, "function");
            }

            vector<Token> reduce_closure_directive(vector<Token> input_tokens) {
                return reduce_directive(input_tokens, "closure");
            }

            vector<Token> reduce_end_directive(vector<Token> input_tokens) {
                return reduce_token_sequence(input_tokens, {".", "end"});
            }

            vector<Token> reduce_signature_directive(vector<Token> input_tokens) {
                return reduce_directive(input_tokens, "signature");
            }

            vector<Token> reduce_bsignature_directive(vector<Token> input_tokens) {
                return reduce_directive(input_tokens, "bsignature");
            }

            vector<Token> reduce_iota_directive(vector<Token> input_tokens) {
                return reduce_directive(input_tokens, "iota");
            }

            vector<Token> reduce_block_directive(vector<Token> input_tokens) {
                return reduce_directive(input_tokens, "block");
            }

            vector<Token> reduce_double_colon(vector<Token> input_tokens) {
                return reduce_token_sequence(input_tokens, {":", ":"});
            }

            vector<Token> reduce_function_signatures(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);
                    if (str::isid(t)) {
                        auto j = i;
                        while (j+2 < limit and input_tokens.at(j+1).str() == "::" and str::isid(input_tokens.at(j+2)) and adjacent(input_tokens.at(j), input_tokens.at(j+1), input_tokens.at(j+2))) {
                            ++j; // swallow "::" token
                            ++j; // swallow identifier token
                        }
                        if (j+1 < limit and input_tokens.at(j+1).str() == "/") {
                            if (j+2 < limit and str::isnum(input_tokens.at(j+2).str(), false)) {
                                if (adjacent(input_tokens.at(j), input_tokens.at(j+1), input_tokens.at(j+2))) {
                                    ++j; // skip "/" token
                                    ++j; // skip integer encoding arity
                                }
                            } else if (j+2 < limit and input_tokens.at(j+2).str() == "\n") {
                                if (adjacent(input_tokens.at(j), input_tokens.at(j+1))) {
                                    ++j; // skip "/" token
                                }
                            }
                            if (j < limit and str::isid(input_tokens.at(j)) and adjacent(input_tokens.at(j-1), input_tokens.at(j))) {
                                ++j;

                            }
                            tokens.emplace_back(t.line(), t.character(), join_tokens(input_tokens, i, j+1));
                            i = j;
                            continue;
                        }
                    }
                    tokens.push_back(t);
                }

                return tokens;
            }

            vector<Token> reduce_names(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);
                    if (str::isid(t)) {
                        auto j = i;
                        while (j+2 < limit and input_tokens.at(j+1).str() == "::" and str::isid(input_tokens.at(j+2)) and adjacent(input_tokens.at(j), input_tokens.at(j+1), input_tokens.at(j+2))) {
                            ++j; // swallow "::" token
                            ++j; // swallow identifier token
                        }
                        if (j+1 < limit and input_tokens.at(j+1).str() == "/") {
                            ++j; // skip "/" token
                            auto next_token = input_tokens.at(j+1).str();
                            if (j+1 < limit and (next_token != "\n" and next_token != "^" and next_token != "(" and next_token != ")" and next_token != "[" and next_token != "]")) {
                                if (adjacent(input_tokens.at(j), input_tokens.at(j+1))) {
                                    ++j; // skip next token, append it to name
                                }
                            }
                            tokens.emplace_back(t.line(), t.character(), join_tokens(input_tokens, i, j+1));
                            i = j;
                            continue;
                        }
                        tokens.emplace_back(t.line(), t.character(), join_tokens(input_tokens, i, j+1));
                        i = j;
                        continue;
                    }
                    tokens.push_back(t);
                }

                return tokens;
            }

            vector<Token> reduce_offset_jumps(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);

                    if (i+1 < limit and (t.str() == "+" or t.str() == "-") and str::isnum(input_tokens.at(i+1))) {
                        if (adjacent(t, input_tokens.at(i+1))) {
                            tokens.emplace_back(t.line(), t.character(), (t.str() + input_tokens.at(i+1).str()));
                            ++i;
                            continue;
                        }
                    }
                    tokens.push_back(t);
                }

                return tokens;
            }

            vector<Token> reduce_at_prefixed_registers(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);

                    if (i+1 < limit and t.str() == "%" and input_tokens.at(i+1).str() != "\n" and is_valid_register_id(input_tokens.at(i+1))) {
                        tokens.emplace_back(t.line(), t.character(), (t.str() + input_tokens.at(i+1).str()));
                        ++i;
                        continue;
                    } else if (i+1 < limit and t.str() == "@" and input_tokens.at(i+1).str() != "\n" and is_valid_register_id(input_tokens.at(i+1))) {
                        tokens.emplace_back(t.line(), t.character(), (t.str() + input_tokens.at(i+1).str()));
                        ++i;
                        continue;
                    } else if (i+1 < limit and t.str() == "*" and input_tokens.at(i+1).str() != "\n" and is_valid_register_id(input_tokens.at(i+1))) {
                        tokens.emplace_back(t.line(), t.character(), (t.str() + input_tokens.at(i+1).str()));
                        ++i;
                        continue;
                    }
                    tokens.push_back(t);
                }

                return tokens;
            }

            vector<Token> reduce_floats(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);
                    if (str::isnum(t) and i < limit-2 and input_tokens.at(i+1) == "." and str::isnum(input_tokens.at(i+2))) {
                        if (adjacent(t, input_tokens.at(i+1), input_tokens.at(i+2))) {
                            tokens.emplace_back(t.line(), t.character(), join_tokens(input_tokens, i, i+3));
                            ++i; // skip "." token
                            ++i; // skip second numeric part
                            continue;
                        }
                    }
                    tokens.push_back(t);
                }

                return tokens;
            }

            vector<Token> move_inline_blocks_out(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;
                decltype(tokens) block_tokens, nested_block_tokens;
                string opened_inside;

                bool block_opened = false;
                bool nested_block_opened = false;

                for (decltype(input_tokens)::size_type i = 0; i < input_tokens.size(); ++i) {
                    const auto& token = input_tokens.at(i);

                    if (token == ".block:" or token == ".function:" or token == ".closure:") {
                        if (block_opened and token == ".block:") {
                            nested_block_opened = true;
                        }
                        if (not nested_block_opened) {
                            opened_inside = input_tokens.at(i+1);
                        }
                        block_opened = true;
                    }

                    if (block_opened) {
                        if (nested_block_opened) {
                            if (nested_block_tokens.size() == 1) {
                                // name is pushed here
                                // must be mangled because we want to allow many functions to have a nested 'foo' block
                                nested_block_tokens.emplace_back(token.line(), token.character(), (opened_inside + "__nested__" + token.str()));
                                nested_block_tokens.back().original(token);
                            } else {
                                nested_block_tokens.push_back(token);
                            }
                        } else {
                            block_tokens.push_back(token);
                        }
                    } else {
                        tokens.push_back(token);
                    }

                    if (token == ".end") {
                        if (nested_block_opened) {
                            for (const auto& ntoken : nested_block_tokens) {
                                tokens.push_back(ntoken);
                            }
                            tokens.emplace_back(nested_block_tokens.front().line(), nested_block_tokens.front().character(), "\n");
                            nested_block_opened = false;

                            // Push nested block name.
                            // This "substitutes" nested block body with block's name.
                            block_tokens.push_back(nested_block_tokens.at(1));

                            nested_block_tokens.clear();
                            continue;
                        }

                        if (block_opened) {
                            for (const auto& btoken : block_tokens) {
                                tokens.push_back(btoken);
                            }
                            block_opened = false;
                            block_tokens.clear();
                            continue;
                        }
                    }
                }

                return tokens;
            }

            static void push_unwrapped_lines(const bool invert, const Token& inner_target_token, vector<Token>& final_tokens, const vector<Token>& unwrapped_tokens, const vector<Token>& input_tokens, std::remove_reference<decltype(input_tokens)>::type::size_type& i) {
                const auto limit = input_tokens.size();
                if (invert) {
                    final_tokens.push_back(inner_target_token);
                    while (i < limit and input_tokens.at(i) != "\n") {
                        final_tokens.push_back(input_tokens.at(i));
                        ++i;
                    }

                    // this line is to push \n to the token list
                    final_tokens.push_back(input_tokens.at(i++));

                    for (auto to : unwrapped_tokens) {
                        final_tokens.push_back(to);
                    }
                } else {
                    vector<Token> last_line;
                    while (final_tokens.size() and final_tokens.at(final_tokens.size()-1) != "\n") {
                        last_line.insert(last_line.begin(), final_tokens.back());
                        final_tokens.pop_back();
                    }

                    for (auto to : unwrapped_tokens) {
                        final_tokens.push_back(to);
                    }

                    for (auto to : last_line) {
                        final_tokens.push_back(to);
                    }
                }
            }
            static void unwrap_subtokens(vector<Token>& unwrapped_tokens, const vector<Token>& subtokens, const Token& token) {
                for (auto subt : unwrap_lines(subtokens)) {
                    unwrapped_tokens.push_back(subt);
                }
                unwrapped_tokens.emplace_back(token.line(), token.character(), "\n");
            }
            static auto get_subtokens(const vector<Token>& input_tokens, std::remove_reference<decltype(input_tokens)>::type::size_type i) -> std::tuple<decltype(i), vector<Token>, unsigned, unsigned, unsigned> {
                string paren_type = input_tokens.at(i);
                string closing_paren_type = ((paren_type == "(") ? ")" : "]");
                ++i;

                vector<Token> subtokens;
                const auto limit = input_tokens.size();

                unsigned balance = 1;
                unsigned toplevel_subexpressions_balance = 0;
                unsigned toplevel_subexpressions = 0;
                while (i < limit) {
                    if (input_tokens.at(i) == paren_type) {
                        ++balance;
                    } else if (input_tokens.at(i) == closing_paren_type) {
                        --balance;
                    }
                    if (input_tokens.at(i) == "(") {
                        if ((++toplevel_subexpressions_balance) == 1) { ++toplevel_subexpressions; }
                    }
                    if (input_tokens.at(i) == ")") {
                        --toplevel_subexpressions_balance;
                    }
                    if (balance == 0) {
                        ++i;
                        break;
                    }
                    subtokens.push_back(input_tokens.at(i));
                    ++i;
                }

                return std::tuple<decltype(i), vector<Token>, unsigned, unsigned, unsigned>{i, std::move(subtokens), balance, toplevel_subexpressions_balance, toplevel_subexpressions};
            }
            static auto get_innermost_target_token(const vector<Token>& subtokens, const Token& t) -> Token {
                Token inner_target_token;
                try {
                    unsigned long check = 1;
                    do {
                        /*
                         * Loop until first operand's token is not "(".
                         * Unwrapping works on second token that appears after the wrap, but
                         * if it is also a "(" then lexer must switch to next second token.
                         * Example:
                         *
                         * iinc (iinc (iinc (iinc (arg 0 1))))
                         *      |     ^     ^     ^    ^
                         *      |     1     3     5    7
                         *      |
                         *       \
                         *       start unwrapping;
                         *       1, 3, and 5 must be skipped because they open inner wraps that
                         *       must be unwrapped before their targets become available
                         *
                         *       skipping by two allows us to fetch the target value without waiting
                         *       for instructions to become unwrapped
                         */
                        inner_target_token = subtokens.at(check);
                        check += 2;
                    } while (inner_target_token.str() == "(");
                    if (inner_target_token == "int"
                        or inner_target_token == "int"
                        or inner_target_token == "int8"
                        or inner_target_token == "int16"
                        or inner_target_token == "int32"
                        or inner_target_token == "int64"
                        or inner_target_token == "uint"
                        or inner_target_token == "uint8"
                        or inner_target_token == "uint16"
                        or inner_target_token == "uint32"
                        or inner_target_token == "uint64"
                        or inner_target_token == "float"
                        or inner_target_token == "float32"
                        or inner_target_token == "float64"
                    ) {
                        inner_target_token = subtokens.at(check-1);
                    }
                } catch (const std::out_of_range& e) {
                    throw InvalidSyntax(t.line(), t.character(), t.str());
                }
                return inner_target_token;
            }
            static auto get_counter_token(const vector<Token>& subtokens, const unsigned toplevel_subexpressions) -> Token {
                return Token{subtokens.at(0).line(), subtokens.at(0).character(), ('%' + str::stringify(toplevel_subexpressions, false))};
            }
            vector<Token> unwrap_lines(vector<Token> input_tokens, bool full) {
                decltype(input_tokens) unwrapped_tokens;
                decltype(input_tokens) tokens;
                decltype(input_tokens) final_tokens;

                decltype(tokens.size()) i = 0, limit = input_tokens.size();
                bool invert = false;

                while (i < limit) {
                    Token t = input_tokens.at(i);
                    if (t == "^") {
                        invert = true;
                        ++i;
                        continue;
                    }
                    if (t == "(" or t == "[") {
                        vector<Token> subtokens;
                        unsigned balance = 1;
                        unsigned toplevel_subexpressions_balance = 0;
                        unsigned toplevel_subexpressions = 0;

                        tie(i, subtokens, balance, toplevel_subexpressions_balance, toplevel_subexpressions) = get_subtokens(input_tokens, i);

                        if (t == "(") {
                            if (i >= limit and balance != 0) {
                                throw InvalidSyntax(t, "unbalanced parenthesis in wrapped instruction");
                            }
                            if (subtokens.size() < 2) {
                                throw InvalidSyntax(t, "at least two tokens are required in a wrapped instruction");
                            }

                            Token inner_target_token = get_innermost_target_token(subtokens, t);
                            unwrap_subtokens(unwrapped_tokens, subtokens, t);
                            push_unwrapped_lines(invert, inner_target_token, final_tokens, unwrapped_tokens, input_tokens, i);
                            if ((not invert) and full) {
                                if (final_tokens.back().str() == "*") {
                                    final_tokens.pop_back();
                                    final_tokens.emplace_back(inner_target_token.line(), inner_target_token.character(), ('*' + inner_target_token.str().substr(1)));
                                    final_tokens.back().original(inner_target_token.str());
                                } else {
                                    final_tokens.push_back(inner_target_token);
                                }
                            }
                        }
                        if (t == "[") {
                            Token counter_token = get_counter_token(subtokens, toplevel_subexpressions);

                            subtokens = unwrap_lines(subtokens, false);

                            unwrap_subtokens(unwrapped_tokens, subtokens, t);
                            push_unwrapped_lines(invert, counter_token, final_tokens, unwrapped_tokens, input_tokens, i);
                            if (not invert) {
                                final_tokens.push_back(counter_token);
                            }
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

            vector<Token> replace_iotas(vector<Token> input_tokens) {
                vector<Token> tokens;
                vector<unsigned long> iotas;

                for (decltype(input_tokens)::size_type i = 0; i < input_tokens.size(); ++i) {
                    const auto& token = input_tokens.at(i);

                    if (token == ".iota:") {
                        if (iotas.empty()) {
                            throw viua::cg::lex::InvalidSyntax(token, "'.iota:' directive used outside of iota scope");
                        }
                        if (not str::isnum(input_tokens.at(i+1))) {
                            throw viua::cg::lex::InvalidSyntax(input_tokens.at(i+1), ("invalid argument to '.iota:' directive: " + str::strencode(input_tokens.at(i+1))));
                        }

                        iotas.back() = stoul(input_tokens.at(i+1));

                        ++i;
                        continue;
                    }

                    if (token == ".function:" or token == ".closure:" or token == ".block:") {
                        iotas.push_back(1);
                    }
                    if (token == "[") {
                        iotas.push_back(0);
                    }
                    if ((token == ".end" or token == "]") and not iotas.empty()) {
                        iotas.pop_back();
                    }

                    if (token == "iota") {
                        tokens.emplace_back(token.line(), token.character(), str::stringify(iotas.back()++, false));
                        tokens.back().original("iota");
                    } else if ((token.str().at(0) == '%' or token.str().at(0) == '@' or token.str().at(0) == '*') and token.str().substr(1) == "iota") {
                        tokens.emplace_back(token.line(), token.character(), (token.str().at(0) + str::stringify(iotas.back()++, false)));
                        tokens.back().original(token);
                    } else {
                        tokens.push_back(token);
                    }
                }

                return tokens;
            }

            vector<Token> replace_defaults(vector<Token> input_tokens) {
                vector<Token> tokens;

                for (decltype(input_tokens)::size_type i = 0; i < input_tokens.size(); ++i) {
                    const auto& token = input_tokens.at(i);
                    tokens.push_back(token);

                    if (match(input_tokens, i, {"arg", "default"}) or match(input_tokens, i, {"call", "default"}) or match(input_tokens, i, {"msg", "default"}) or match(input_tokens, i, {"process", "default"})) {
                        ++i;
                        tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "void");
                        tokens.back().original("default");
                        continue;
                    }
                    if (match(input_tokens, i, {"istore", "", "default"})) {
                        tokens.push_back(input_tokens.at(++i));  // push target register token
                        ++i;
                        tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "0");
                        tokens.back().original("default");
                        continue;
                    }
                    if (match(input_tokens, i, {"fstore", "", "default"})) {
                        tokens.push_back(input_tokens.at(++i));  // push target register token
                        ++i;
                        tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "0.0");
                        tokens.back().original("default");
                        continue;
                    }
                    if (match(input_tokens, i, {"strstore", "", "default"})) {
                        tokens.push_back(input_tokens.at(++i));  // push target register token
                        ++i;
                        tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "\"\"");
                        tokens.back().original("default");
                        continue;
                    }
                    if (match(input_tokens, i, {"receive", "", "default"})) {
                        ++i;
                        if (input_tokens.at(i) == "default") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "void");
                            tokens.back().original("default");
                        } else {
                            tokens.push_back(input_tokens.at(i));  // push target register token
                        }
                        ++i;
                        tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "infinity");
                        tokens.back().original("default");
                        continue;
                    }
                    if (match(input_tokens, i, {"join", "", "", "default"})) {
                        ++i;
                        if (input_tokens.at(i) == "default") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "void");
                            tokens.back().original("default");
                        } else {
                            tokens.push_back(input_tokens.at(i));  // push target register token
                        }
                        tokens.push_back(input_tokens.at(++i));  // push source register token
                        ++i;
                        tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "infinity");
                        tokens.back().original("default");
                        continue;
                    }
                }

                return tokens;
            }

            std::vector<Token> replace_named_registers(std::vector<Token> input_tokens) {
                vector<Token> tokens;
                map<string, string> names;
                unsigned open_blocks = 0;

                for (decltype(input_tokens)::size_type i = 0; i < input_tokens.size(); ++i) {
                    const auto& token = input_tokens.at(i);

                    if (str::isnum(token)) {
                        tokens.push_back(token);
                        continue;
                    }

                    if (token == ".function:" or token == ".closure:" or token == ".block:") {
                        ++open_blocks;
                    }
                    if (token == ".end") {
                        --open_blocks;
                    }

                    if (token == ".name:") {
                        Token name = input_tokens.at(i+2);
                        string index = input_tokens.at(i+1).str();

                        assert_is_not_reserved_keyword(name, "register name");

                        if ((index.at(0) == '%' or index.at(0) == '@' or index.at(0) == '*') and str::isnum(index.substr(1), false)) {
                            index = index.substr(1);
                        }

                        if (not str::isnum(index)) {
                            throw viua::cg::lex::InvalidSyntax(input_tokens.at(i+1), ("invalid register index: " + str::strencode(name) + " := " + str::enquote(str::strencode(index))));
                        }

                        names[name] = index;
                        i += 2;
                        continue;
                    }

                    if (names.count(token)) {
                        tokens.emplace_back(token.line(), token.character(), names.at(token));
                        tokens.back().original(token.str());
                    } else if (token.str().at(0) == '@' and names.count(token.str().substr(1))) {
                        tokens.emplace_back(token.line(), token.character(), ("@" + names.at(token.str().substr(1))));
                        tokens.back().original(token.str().substr(1));
                    } else if (token.str().at(0) == '*' and names.count(token.str().substr(1))) {
                        tokens.emplace_back(token.line(), token.character(), ("*" + names.at(token.str().substr(1))));
                        tokens.back().original(token.str().substr(1));
                    } else if (token.str().at(0) == '%' and names.count(token.str().substr(1))) {
                        tokens.emplace_back(token.line(), token.character(), ("%" + names.at(token.str().substr(1))));
                        tokens.back().original(token.str().substr(1));
                    } else {
                        tokens.push_back(token);
                    }

                    if (open_blocks == 0) {
                        names.clear();
                    }
                }

                return tokens;
            }

            vector<Token> cook(vector<Token> tokens, const bool with_replaced_names) {
                /*
                 * Remove whitespace as first step to reduce noise in token stream.
                 * Remember not to remove newlines ('\n') because they act as separators
                 * in Viua assembly language, and are thus quite important.
                 */
                tokens = remove_spaces(tokens);
                tokens = remove_comments(tokens);

                /*
                 * Reduce consecutive newline tokens to a single newline.
                 * For example, ["foo", "\n", "\n", "\n", "bar"] is reduced to ["foo", "\n", "bar"].
                 * This further reduces noise, and allows later reductions to make the assumption
                 * that there is always only one newline when newline may appear.
                 */
                tokens = reduce_newlines(tokens);

                /*
                 * Reduce directives as lexer emits them as multiple tokens.
                 * This lets later reductions to check jsut one token to see if it is an assembler
                 * directive instead of looking two or three tokens ahead.
                 */
                tokens = reduce_function_directive(tokens);
                tokens = reduce_closure_directive(tokens);
                tokens = reduce_end_directive(tokens);
                tokens = reduce_signature_directive(tokens);
                tokens = reduce_bsignature_directive(tokens);
                tokens = reduce_block_directive(tokens);
                tokens = reduce_info_directive(tokens);
                tokens = reduce_name_directive(tokens);
                tokens = reduce_main_directive(tokens);
                tokens = reduce_link_directive(tokens);
                tokens = reduce_mark_directive(tokens);
                tokens = reduce_iota_directive(tokens);

                /*
                 * Reduce directive-looking strings.
                 */
                tokens = reduce_token_sequence(tokens, {".", "", ":"});

                /*
                 * Reduce double-colon token to make life easier for name reductions.
                 */
                tokens = reduce_double_colon(tokens);

                /*
                 * Then, reduce function signatues and names.
                 * Reduce function names first because they have the arity suffix ('/<integer>') apart
                 * from the 'name ("::" name)*' core which both reducers recognise.
                 */
                tokens = reduce_function_signatures(tokens);
                tokens = reduce_names(tokens);

                /*
                 * Reduce other tokens that are not lexed as single entities, e.g. floating-point literals, or
                 * @-prefixed registers.
                 *
                 * The order should not be changed as the functions make assumptions about input list, and
                 * parsing may break if the assumptions are false.
                 * Order may be changed if the externally visible outputs from assembler (i.e. compiled bytecode)
                 * do not change.
                 */
                tokens = reduce_offset_jumps(tokens);
                tokens = reduce_at_prefixed_registers(tokens);
                tokens = reduce_floats(tokens);

                /*
                 * Replace 'iota' keywords with their integers.
                 * This **MUST** be run before unwrapping instructions because '[]' are needed to correctly
                 * replace iotas inside them (the '[]' create new iota scopes).
                 */
                tokens = replace_iotas(tokens);

                /*
                 * Replace 'default' keywords with their values.
                 * This **MUST** be run before unwrapping instructions because unwrapping copies and
                 * rearranges tokens in a list so "default" may be copied somewhere where the expansion would be
                 * incorrect, or illegal.
                 */
                tokens = replace_defaults(tokens);

                /*
                 * Unroll instruction wrapped in '()' and '[]'.
                 * This makes assembler's and static analyser's work easier since they can deal with linear
                 * token sequence.
                 */
                tokens = unwrap_lines(tokens);

                /*
                 * Reduce @- and *-prefixed registers once more.
                 * This is to support constructions like this:
                 *
                 *  print *(ptr X A)
                 *
                 * where the prefix and register name are disconnected before the lines are unwrapped.
                 */
                tokens = reduce_at_prefixed_registers(tokens);

                /*
                 * Replace register names set by '.name:' directive by their register indexes.
                 * At later processing stages functions need not concern themselves with names and
                 * may operate on register indexes only.
                 */
                if (with_replaced_names) {
                    tokens = replace_named_registers(tokens);
                }

                /*
                 * Move inlined blocks out of their functions.
                 * This makes life easier for functions at later processing stages as they do not have to deal with
                 * nested blocks.
                 *
                 * Still, this must be run after iota expansion because nested blocks should share iotas with their
                 * functions.
                 */
                tokens = move_inline_blocks_out(tokens);

                /*
                 * Reduce newlines once more, since unwrap_lines() may sometimes insert a spurious newline into the
                 * token stream.
                 * It's easier to just clean the newlines up after unwrap_lines() than to add extra ifs to it, and
                 * it also helps readability (unwrap_lines() may be less convulted).
                 */
                tokens = reduce_newlines(tokens);

                return tokens;
            }
        }
    }
}
