#include <sstream>
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

            Token::Token(decltype(line_number) line_, decltype(character_in_line) character_, string content_): content(content_), line_number(line_), character_in_line(character_) {
            }

            const char* InvalidSyntax::what() const {
                return message.c_str();
            }

            auto InvalidSyntax::line() const -> decltype(line_number) {
                return line_number;
            }
            auto InvalidSyntax::character() const -> decltype(character_in_line) {
                return character_in_line;
            }

            InvalidSyntax::InvalidSyntax(long unsigned ln, long unsigned ch, string ct): line_number(ln), character_in_line(ch), content(ct) {
            }
            InvalidSyntax::InvalidSyntax(Token t, string m): line_number(t.line()), character_in_line(t.character()), content(t.str()), message(m) {
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

            vector<Token> standardise(vector<Token> input_tokens) {
                vector<Token> tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    Token token = input_tokens.at(i);
                    if (token == "call" or token == "process") {
                        tokens.push_back(token);
                        if ((not str::isnum(input_tokens.at(i+1).str(), false)) and input_tokens.at(i+2).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "0");
                        }
                    } else if (token == "frame") {
                        tokens.push_back(token);

                        if ((not str::isnum(input_tokens.at(i+1).str(), false)) and input_tokens.at(i+1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "0");
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "16");
                            continue;
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if ((not str::isnum(input_tokens.at(i+1).str(), false)) and input_tokens.at(i+1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "16");
                        }
                    } else if (token == "vec") {
                        tokens.push_back(token);
                        tokens.push_back(input_tokens.at(++i));

                        if ((not str::isnum(input_tokens.at(i+1).str(), false)) and input_tokens.at(i+1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "0"); // starting register
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "0"); // number of registers to pack
                            continue;
                        }

                        tokens.push_back(input_tokens.at(++i)); // starting register
                        if ((not str::isnum(input_tokens.at(i+1).str(), false)) and input_tokens.at(i+1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "0"); // number of registers to pack
                        }
                    } else if (token == "vpop") {
                        tokens.push_back(token);
                        tokens.push_back(input_tokens.at(++i));

                        if ((not str::isnum(input_tokens.at(i+1).str(), false)) and input_tokens.at(i+1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "0");
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "-1");
                            continue;
                        }

                        tokens.push_back(input_tokens.at(++i));
                        if ((not str::isnum(input_tokens.at(i+1).str(), false)) and input_tokens.at(i+1).str() == "\n") {
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "-1");
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
                    } else if (token == "iadd" or token == "isub" or token == "imul" or token == "idiv" or
                               token == "ilt" or token == "ilte" or token == "igt" or token == "igte" or token == "ieq" or
                               token == "fadd" or token == "fsub" or token == "fmul" or token == "fdiv" or
                               token == "flt" or token == "flte" or token == "fgt" or token == "fgte" or token == "feq" or
                               token == "and" or token == "or"
                    ) {
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
                    } else if (token == "enclose" or token == "enclosecopy" or token == "enclosemove") {
                        tokens.push_back(token);    // mnemonic
                        tokens.push_back(input_tokens.at(++i)); // target register
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
                        } else {
                            tokens.push_back(input_tokens.at(++i)); // source register
                        }
                        continue;
                    } else if (token == "istore") {
                        tokens.push_back(token);                // mnemonic
                        tokens.push_back(input_tokens.at(++i)); // target register
                        if (input_tokens.at(i+1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "0");
                        }
                        continue;
                    } else if (token == "fstore") {
                        tokens.push_back(token);                // mnemonic
                        tokens.push_back(input_tokens.at(++i)); // target register
                        if (input_tokens.at(i+1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "0.0");
                        }
                        continue;
                    } else if (token == "strstore") {
                        tokens.push_back(token);                // mnemonic
                        tokens.push_back(input_tokens.at(++i)); // target register
                        if (input_tokens.at(i+1) == "\n") {
                            tokens.emplace_back(input_tokens.at(i).line(), input_tokens.at(i).character(), "\"\"");
                        }
                        continue;
                    } else if (token == "itof" or token == "ftoi" or token == "stoi" or token == "stof") {
                        tokens.push_back(token);                // mnemonic
                        tokens.push_back(input_tokens.at(++i)); // target register

                        if (input_tokens.at(i+1).str() == "\n") {
                            tokens.push_back(tokens.back());
                        } else {
                            tokens.push_back(input_tokens.at(++i));
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

            vector<Token> reduce_mark_directive(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);
                    if (t.str() == "." and i < limit-2 and input_tokens.at(i+1) == "mark" and input_tokens.at(i+2).str() == ":") {
                        if (adjacent(t, input_tokens.at(i+1), input_tokens.at(i+2))) {
                            tokens.emplace_back(t.line(), t.character(), ".mark:");
                            ++i; // skip "mark" token
                            ++i; // skip ":" token
                            continue;
                        }
                    }
                    tokens.push_back(t);
                }

                return tokens;
            }

            vector<Token> reduce_name_directive(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);
                    if (t.str() == "." and i < limit-2 and input_tokens.at(i+1) == "name" and input_tokens.at(i+2).str() == ":") {
                        if (adjacent(t, input_tokens.at(i+1), input_tokens.at(i+2))) {
                            tokens.emplace_back(t.line(), t.character(), ".name:");
                            ++i; // skip "name" token
                            ++i; // skip ":" token
                            continue;
                        }
                    }
                    tokens.push_back(t);
                }

                return tokens;
            }

            vector<Token> reduce_info_directive(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);
                    if (t.str() == "." and i < limit-2 and input_tokens.at(i+1) == "info" and input_tokens.at(i+2).str() == ":") {
                        if (adjacent(t, input_tokens.at(i+1), input_tokens.at(i+2))) {
                            tokens.emplace_back(t.line(), t.character(), ".info:");
                            ++i; // skip "info" token
                            ++i; // skip ":" token
                            continue;
                        }
                    }
                    tokens.push_back(t);
                }

                return tokens;
            }

            vector<Token> reduce_main_directive(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);
                    if (t.str() == "." and i < limit-2 and input_tokens.at(i+1) == "main" and input_tokens.at(i+2).str() == ":") {
                        if (adjacent(t, input_tokens.at(i+1), input_tokens.at(i+2))) {
                            tokens.emplace_back(t.line(), t.character(), ".main:");
                            ++i; // skip "main" token
                            ++i; // skip ":" token
                            continue;
                        }
                    }
                    tokens.push_back(t);
                }

                return tokens;
            }

            vector<Token> reduce_link_directive(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);
                    if (t.str() == "." and i < limit-2 and input_tokens.at(i+1) == "link" and input_tokens.at(i+2).str() == ":") {
                        if (adjacent(t, input_tokens.at(i+1), input_tokens.at(i+2))) {
                            tokens.emplace_back(t.line(), t.character(), ".link:");
                            ++i; // skip "link" token
                            ++i; // skip ":" token
                            continue;
                        }
                    }
                    tokens.push_back(t);
                }

                return tokens;
            }

            vector<Token> reduce_function_directive(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);
                    if (t.str() == "." and i < limit-2 and input_tokens.at(i+1) == "function" and input_tokens.at(i+2).str() == ":") {
                        if (adjacent(t, input_tokens.at(i+1), input_tokens.at(i+2))) {
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

            vector<Token> reduce_closure_directive(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);
                    if (t.str() == "." and i < limit-2 and input_tokens.at(i+1) == "closure" and input_tokens.at(i+2).str() == ":") {
                        if (adjacent(t, input_tokens.at(i+1), input_tokens.at(i+2))) {
                            tokens.emplace_back(t.line(), t.character(), ".closure:");
                            ++i; // skip "closure" token
                            ++i; // skip ":" token
                            continue;
                        }
                    }
                    tokens.push_back(t);
                }

                return tokens;
            }

            vector<Token> reduce_end_directive(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);
                    if (t.str() == "." and i < limit-1 and input_tokens.at(i+1) == "end") {
                        if (adjacent(t, input_tokens.at(i+1))) {
                            tokens.emplace_back(t.line(), t.character(), ".end");
                            ++i; // skip "end" token
                            continue;
                        }
                    }
                    tokens.push_back(t);
                }

                return tokens;
            }

            vector<Token> reduce_signature_directive(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);
                    if (t.str() == "." and i < limit-2 and input_tokens.at(i+1) == "signature" and input_tokens.at(i+2).str() == ":") {
                        if (adjacent(t, input_tokens.at(i+1), input_tokens.at(i+2))) {
                            tokens.emplace_back(t.line(), t.character(), ".signature:");
                            ++i; // skip "signature" token
                            ++i; // skip ":" token
                            continue;
                        }
                    }
                    tokens.push_back(t);
                }

                return tokens;
            }

            vector<Token> reduce_bsignature_directive(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);
                    if (t.str() == "." and i < limit-2 and input_tokens.at(i+1) == "bsignature" and input_tokens.at(i+2).str() == ":") {
                        if (adjacent(t, input_tokens.at(i+1), input_tokens.at(i+2))) {
                            tokens.emplace_back(t.line(), t.character(), ".bsignature:");
                            ++i; // skip "bsignature" token
                            ++i; // skip ":" token
                            continue;
                        }
                    }
                    tokens.push_back(t);
                }

                return tokens;
            }

            vector<Token> reduce_iota_directive(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);
                    if (t.str() == "." and i < limit-2 and input_tokens.at(i+1) == "iota" and input_tokens.at(i+2).str() == ":") {
                        if (adjacent(t, input_tokens.at(i+1), input_tokens.at(i+2))) {
                            tokens.emplace_back(t.line(), t.character(), ".iota:");
                            ++i; // skip "iota" token
                            ++i; // skip ":" token
                            continue;
                        }
                    }
                    tokens.push_back(t);
                }

                return tokens;
            }

            vector<Token> reduce_block_directive(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);
                    if (t.str() == "." and i < limit-2 and input_tokens.at(i+1) == "block" and input_tokens.at(i+2).str() == ":") {
                        if (adjacent(t, input_tokens.at(i+1), input_tokens.at(i+2))) {
                            tokens.emplace_back(t.line(), t.character(), ".block:");
                            ++i; // skip "bsignature" token
                            ++i; // skip ":" token
                            continue;
                        }
                    }
                    tokens.push_back(t);
                }

                return tokens;
            }

            vector<Token> reduce_double_colon(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);
                    if (t.str() == ":" and i < limit-1 and input_tokens.at(i+1).str() == ":") {
                        if (adjacent(t, input_tokens.at(i+1))) {
                            tokens.emplace_back(t.line(), t.character(), "::");
                            ++i; // skip second ":" token
                            continue;
                        }
                    }
                    tokens.push_back(t);
                }

                return tokens;
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

                    if (i+1 < limit and t.str() == "@" and input_tokens.at(i+1).str() != "\n" and adjacent(t, input_tokens.at(i+1))) {
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

            vector<Token> reduce_absolute_jumps(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens.at(i);

                    if (i+1 < limit and t == "." and str::isnum(input_tokens.at(i+1))) {
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

            vector<Token> unwrap_lines(vector<Token> input_tokens, bool full) {
                // FIXME: this function need refactoring
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
                    if (t == "(") {
                        vector<Token> subtokens;
                        ++i;
                        unsigned balance = 1;
                        while (i < limit) {
                            if (input_tokens.at(i) == "(") {
                                ++balance;
                            } else if (input_tokens.at(i) == ")") {
                                --balance;
                            }
                            if (balance == 0) {
                                ++i;
                                break;
                            }
                            subtokens.push_back(input_tokens.at(i));
                            ++i;
                        }
                        if (i >= limit and balance != 0) {
                            throw InvalidSyntax(t, "unbalanced parenthesis in wrapped instruction");
                        }
                        if (subtokens.size() < 2) {
                            throw InvalidSyntax(t, "at least two tokens are required in a wrapped instruction");
                        }

                        Token tok;
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
                                tok = subtokens.at(check);
                                check += 2;
                            } while (tok.str() == "(");
                        } catch (const std::out_of_range& e) {
                            throw InvalidSyntax(t.line(), t.character(), t.str());
                        }

                        for (auto subt : unwrap_lines(subtokens)) {
                            unwrapped_tokens.push_back(subt);
                        }
                        unwrapped_tokens.emplace_back(t.line(), t.character(), "\n");

                        if (invert) {
                            final_tokens.push_back(tok);
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
                            vector<Token> foo;
                            while (final_tokens.size() and final_tokens.at(final_tokens.size()-1) != "\n") {
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
                            if (input_tokens.at(i) == "[") {
                                ++balance;
                            } else if (input_tokens.at(i) == "]") {
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

                        Token counter_token(subtokens.at(0).line(), subtokens.at(0).character(), str::stringify(toplevel_subexpressions, false));

                        subtokens = unwrap_lines(subtokens, false);

                        for (auto subt : unwrap_lines(subtokens)) {
                            unwrapped_tokens.push_back(subt);
                        }
                        unwrapped_tokens.emplace_back(t.line(), t.character(), "\n");

                        if (invert) {
                            final_tokens.push_back(counter_token);
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
                            vector<Token> foo;
                            while (final_tokens.size() and final_tokens.at(final_tokens.size()-1) != "\n") {
                                foo.insert(foo.begin(), final_tokens.back());
                                final_tokens.pop_back();
                            }

                            for (auto to : unwrapped_tokens) {
                                final_tokens.push_back(to);
                            }

                            for (auto to : foo) {
                                final_tokens.push_back(to);
                            }
                            final_tokens.push_back(counter_token);
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
                    } else {
                        tokens.push_back(token);
                    }
                }

                return tokens;
            }

            vector<Token> reduce(vector<Token> tokens) {
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
                 *
                 * Also, reduce other multi-token sequences: function, block, and module names;
                 * floating point numbers; absolute jumps; @-prefixed register names and indexes.
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
                tokens = reduce_absolute_jumps(tokens);

                /*
                 * Replace 'iota' keywords with their integers.
                 * This **MUST** be run before unwrapping instructions because '[]' are needed to correctly
                 * replace iotas inside them (the '[]' create new iota scopes).
                 */
                tokens = replace_iotas(tokens);

                /*
                 * Unroll instruction wrapped in '()' and '[]'.
                 * This makes assembler's and static analyser's work easier since they can deal with linear
                 * token sequence.
                 */
                tokens = unwrap_lines(tokens);

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
