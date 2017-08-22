/*
 *  Copyright (C) 2017 Marek Marecki
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

#ifndef VIUA_ASSEMBLER_UTIL_PRETTY_PRINTER_H
#define VIUA_ASSEMBLER_UTIL_PRETTY_PRINTER_H

#include <string>
#include <vector>
#include <viua/cg/lex.h>

namespace viua {
    namespace assembler {
        namespace util {
            namespace pretty_printer {
                const std::string COLOR_FG_RED = "\x1b[38;5;1m";
                const std::string COLOR_FG_YELLOW = "\x1b[38;5;3m";
                const std::string COLOR_FG_CYAN = "\x1b[38;5;6m";
                const std::string COLOR_FG_LIGHT_GREEN = "\x1b[38;5;10m";
                const std::string COLOR_FG_LIGHT_YELLOW = "\x1b[38;5;11m";
                const std::string COLOR_FG_WHITE = "\x1b[38;5;15m";
                const std::string COLOR_FG_GREEN_1 = "\x1b[38;5;46m";
                const std::string COLOR_FG_RED_1 = "\x1b[38;5;196m";
                const std::string COLOR_FG_ORANGE_RED_1 = "\x1b[38;5;202m";
                const std::string ATTR_RESET = "\x1b[0m";

                auto send_control_seq(const std::string&) -> std::string;

                auto underline_error_token(const std::vector<viua::cg::lex::Token>& tokens,
                                           decltype(tokens.size()) i,
                                           const viua::cg::lex::InvalidSyntax& error) -> void;
                auto display_error_line(const std::vector<viua::cg::lex::Token>& tokens,
                                        const viua::cg::lex::InvalidSyntax& error, decltype(tokens.size()) i)
                    -> decltype(i);
                auto display_context_line(const std::vector<viua::cg::lex::Token>& tokens,
                                          const viua::cg::lex::InvalidSyntax&, decltype(tokens.size()) i)
                    -> decltype(i);
                auto display_error_header(const viua::cg::lex::InvalidSyntax& error,
                                          const std::string& filename) -> void;
                auto display_error_location(const std::vector<viua::cg::lex::Token>& tokens,
                                            const viua::cg::lex::InvalidSyntax error) -> void;
                auto display_error_in_context(const std::vector<viua::cg::lex::Token>& tokens,
                                              const viua::cg::lex::InvalidSyntax error,
                                              const std::string& filename) -> void;
                auto display_error_in_context(const std::vector<viua::cg::lex::Token>& tokens,
                                              const viua::cg::lex::TracedSyntaxError error,
                                              const std::string& filename) -> void;
            }
        }
    }
}

#endif
