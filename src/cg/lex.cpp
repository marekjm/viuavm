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

            auto Token::ends() const -> decltype(character_in_line) {
                return (character_in_line + content.size());
            }

            bool Token::operator==(const string& s) {
                return (content == s);
            }
            bool Token::operator!=(const string& s) {
                return (content != s);
            }

            Token::Token(decltype(line_number) line_, decltype(character_in_line) character_, string content_): content(content_), line_number(line_), character_in_line(character_) {
            }

            InvalidSyntax::InvalidSyntax(long unsigned ln, long unsigned ch, string ct): line_number(ln), character_in_line(ch), content(ct) {
            }

            vector<Token> tokenise(const string& source) {
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

            vector<Token> standardise(vector<Token> input_tokens) {
                vector<Token> tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    Token token = input_tokens.at(i);
                    if (token == "call" or token == "process") {
                        tokens.push_back(token);
                        if (not str::isnum(input_tokens.at(i+1).str(), false)) {
                            tokens.emplace_back(input_tokens.at(i+1).line(), input_tokens.at(i+1).character(), "0");
                        }
                    } else {
                        tokens.push_back(token);
                    }
                }

                return tokens;
            }

            string join_tokens(const vector<Token> tokens, const decltype(tokens)::size_type from, const decltype(from) to) {
                ostringstream joined;

                for (auto i = from; i < tokens.size() and i < to; ++i) {
                    joined << tokens[i].str();
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

            vector<Token> reduce_newlines(vector<Token> input_tokens) {
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

            vector<Token> reduce_function_directive(vector<Token> input_tokens) {
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

            vector<Token> reduce_end_directive(vector<Token> input_tokens) {
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

            vector<Token> reduce_signature_directive(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens[i];
                    if (t.str() == "." and i < limit-2 and input_tokens[i+1] == "signature" and input_tokens[i+2].str() == ":") {
                        if (adjacent(t, input_tokens[i+1], input_tokens[i+2])) {
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

            vector<Token> reduce_double_colon(vector<Token> input_tokens) {
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

            vector<Token> reduce_function_signatures(vector<Token> input_tokens) {
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

            vector<Token> unwrap_lines(vector<Token> input_tokens, bool full) {
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

            vector<Token> reduce(vector<Token> tokens) {
                return reduce_signature_directive(reduce_function_signatures(reduce_double_colon(reduce_end_directive(reduce_function_directive(unwrap_lines(reduce_newlines(remove_comments(remove_spaces(tokens)))))))));
            }
        }
    }
}
