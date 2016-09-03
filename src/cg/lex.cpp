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
                    char current_char = source[i];
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
                            candidate_token << source[i];
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

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    Token token = input_tokens[i];
                    if (token.str() == ";") {
                        // FIXME this is ugly as hell
                        do {
                            ++i;
                        } while (i < limit and input_tokens[i].str() != "\n");
                        if (i < limit) {
                            tokens.push_back(input_tokens[i]);
                        }
                    } else if (i+2 < limit and adjacent(token, input_tokens.at(i+1)) and token.str() == "-" and input_tokens.at(i+1).str() == "-") {
                        // FIXME this is ugly as hell
                        do {
                            ++i;
                        } while (i < limit and input_tokens[i].str() != "\n");
                        if (i < limit) {
                            tokens.push_back(input_tokens[i]);
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
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    Token token = input_tokens[i];
                    if (token.str() == "\n") {
                        tokens.push_back(token);
                        while (i < limit and input_tokens[i].str() == "\n") {
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
                    const auto t = input_tokens[i];
                    if (t.str() == "." and i < limit-2 and input_tokens[i+1] == "mark" and input_tokens[i+2].str() == ":") {
                        if (adjacent(t, input_tokens[i+1], input_tokens[i+2])) {
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
                    const auto t = input_tokens[i];
                    if (t.str() == "." and i < limit-2 and input_tokens[i+1] == "name" and input_tokens[i+2].str() == ":") {
                        if (adjacent(t, input_tokens[i+1], input_tokens[i+2])) {
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

            vector<Token> reduce_bsignature_directive(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens[i];
                    if (t.str() == "." and i < limit-2 and input_tokens[i+1] == "bsignature" and input_tokens[i+2].str() == ":") {
                        if (adjacent(t, input_tokens[i+1], input_tokens[i+2])) {
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

            vector<Token> reduce_block_directive(vector<Token> input_tokens) {
                decltype(input_tokens) tokens;

                const auto limit = input_tokens.size();
                for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
                    const auto t = input_tokens[i];
                    if (t.str() == "." and i < limit-2 and input_tokens[i+1] == "block" and input_tokens[i+2].str() == ":") {
                        if (adjacent(t, input_tokens[i+1], input_tokens[i+2])) {
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
                    if (str::isid(t)) {
                        auto j = i;
                        while (j+2 < limit and input_tokens[j+1].str() == "::" and str::isid(input_tokens[j+2]) and adjacent(input_tokens[j], input_tokens[j+1], input_tokens[j+2])) {
                            ++j; // swallow "::" token
                            ++j; // swallow identifier token
                        }
                        if (j+1 < limit and input_tokens[j+1].str() == "/") {
                            if (j+2 < limit and str::isnum(input_tokens[j+2].str(), false)) {
                                if (adjacent(input_tokens[j], input_tokens[j+1], input_tokens[j+2])) {
                                    ++j; // skip "/" token
                                    ++j; // skip integer encoding arity
                                }
                            } else if (j+2 < limit and input_tokens[j+2].str() == "\n") {
                                if (adjacent(input_tokens[j], input_tokens[j+1])) {
                                    ++j; // skip "/" token
                                }
                            }
                            if (j < limit and str::isid(input_tokens[j]) and adjacent(input_tokens[j-1], input_tokens[j])) {
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
                    const auto t = input_tokens[i];
                    if (str::isid(t)) {
                        auto j = i;
                        while (j+2 < limit and input_tokens[j+1].str() == "::" and str::isid(input_tokens[j+2]) and adjacent(input_tokens[j], input_tokens[j+1], input_tokens[j+2])) {
                            ++j; // swallow "::" token
                            ++j; // swallow identifier token
                        }
                        if (j+1 < limit and input_tokens[j+1].str() == "/") {
                            ++j; // skip "/" token
                            if (j+1 < limit and input_tokens[j+1].str() != "\n") {
                                if (adjacent(input_tokens[j], input_tokens[j+1])) {
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
                    const auto t = input_tokens[i];

                    if (i+1 < limit and (t.str() == "+" or t.str() == "-") and str::isnum(input_tokens[i+1])) {
                        if (adjacent(t, input_tokens[i+1])) {
                            tokens.emplace_back(t.line(), t.character(), (t.str() + input_tokens[i+1].str()));
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
                    const auto t = input_tokens[i];

                    if (i+1 < limit and t.str() == "@" and input_tokens[i+1].str() != "\n" and adjacent(t, input_tokens[i+1])) {
                        tokens.emplace_back(t.line(), t.character(), (t.str() + input_tokens[i+1].str()));
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
                    const auto t = input_tokens[i];
                    if (str::isnum(t) and i < limit-2 and input_tokens[i+1] == "." and str::isnum(input_tokens[i+2])) {
                        if (adjacent(t, input_tokens[i+1], input_tokens[i+2])) {
                            tokens.emplace_back(t.line(), t.character(), join_tokens(input_tokens, i, i+3));
                            ++i; // skip "name" token
                            ++i; // skip ":" token
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
                    const auto t = input_tokens[i];

                    if (i+1 < limit and t == "." and str::isnum(input_tokens[i+1], false)) {
                        if (adjacent(t, input_tokens[i+1])) {
                            tokens.emplace_back(t.line(), t.character(), (t.str() + input_tokens[i+1].str()));
                            ++i;
                            continue;
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
                return reduce_absolute_jumps(reduce_floats(reduce_at_prefixed_registers(reduce_offset_jumps(reduce_mark_directive(reduce_name_directive(reduce_block_directive(reduce_bsignature_directive(reduce_signature_directive(reduce_names(reduce_function_signatures(reduce_double_colon(reduce_end_directive(reduce_function_directive(unwrap_lines(reduce_newlines(remove_comments(remove_spaces(tokens))))))))))))))))));
            }
        }
    }
}
