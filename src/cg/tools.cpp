#include <viua/cg/tools.h>
using namespace std;


namespace viua {
    namespace cg {
        namespace tools {
            OPCODE mnemonic_to_opcode(const string& s) {
                OPCODE op = NOP;
                bool found = false;
                for (auto it : OP_NAMES) {
                    if (it.second == s) {
                        found = true;
                        op = it.first;
                        break;
                    }
                }
                if (not found) {
                    throw std::out_of_range("invalid instruction name: " + s);
                }
                return op;
            }
            uint64_t calculate_bytecode_size(const vector<viua::cg::lex::Token>& tokens) {
                uint64_t bytes = 0, inc = 0;

                const auto limit = tokens.size();
                for (decltype(tokens.size()) i = 0; i < limit; ++i) {
                    viua::cg::lex::Token token = tokens[i];
                    if (token.str().substr(0, 1) == ".") {
                        while (i < limit and tokens[i].str() != "\n") {
                            ++i;
                        }
                        continue;
                    }
                    if (token.str() == "\n") {
                        continue;
                    }
                    OPCODE op;
                    try {
                        op = mnemonic_to_opcode(token.str());
                        inc = OP_SIZES.at(token.str());
                        if (any(op, ENTER, LINK, WATCHDOG, TAILCALL)) {
                            // get second chunk (function, block or module name)
                            inc += (tokens.at(i+1).str().size() + 1);
                        } else if (any(op, CALL, MSG, PROCESS)) {
                            ++i; // skip register index
                            if (tokens.at(i+1).str() == "\n") {
                                throw viua::cg::lex::InvalidSyntax(token.line(), token.character(), token.str());
                            }
                            inc += (tokens.at(i+1).str().size() + 1);
                        } else if (any(op, CLOSURE, FUNCTION, CLASS, PROTOTYPE, DERIVE, NEW)) {
                            ++i; // skip register index
                            if (tokens.at(i+1).str() == "\n") {
                                throw viua::cg::lex::InvalidSyntax(token.line(), token.character(), token.str());
                            }
                            inc += (tokens.at(i+1).str().size() + 1);
                        } else if (op == ATTACH) {
                            ++i; // skip register index
                            inc += (tokens[++i].str().size() + 1);
                            inc += (tokens[++i].str().size() + 1);
                        } else if (op == IMPORT) {
                            inc += (tokens[++i].str().size() - 2 + 1);
                        } else if (op == CATCH) {
                            inc += (tokens[++i].str().size() - 2 + 1); // +1: null-terminator, -2: quotes
                            inc += (tokens[++i].str().size() + 1);
                        } else if (op == STRSTORE) {
                            ++i; // skip register index
                            inc += (tokens[++i].str().size() - 2 + 1 );
                        }
                    } catch (const std::out_of_range& e) {
                        throw viua::cg::lex::InvalidSyntax(token.line(), token.character(), token.str());
                    }

                    // skip tokens until "\n" after an instruction has been counted
                    while (i < limit and tokens[i].str() != "\n") {
                        ++i;
                    }

                    bytes += inc;
                }

                return bytes;
            }
        }
    }
}
