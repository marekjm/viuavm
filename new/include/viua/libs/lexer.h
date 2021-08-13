/*
 *  Copyright (C) 2021 Marek Marecki
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

#ifndef VIUA_LIBS_LEXER_H
#define VIUA_LIBS_LEXER_H

#include <stdint.h>

#include <string>
#include <string_view>
#include <vector>


namespace viua::libs::lexer {
    struct Location {
        size_t line {};
        size_t character {};
        size_t offset {};
    };

    enum class TOKEN {
        /*
         * Compiler and assembler directives, instructions.
         */
        IMPORT,
        EXPORT,
        OPCODE,

        /*
         * Control characters.
         */
        COMMA,
        EQ,
        TERMINATOR,
        ATTR_LIST_OPEN,
        ATTR_LIST_CLOSE,
        PAREN_OPEN,
        PAREN_CLOSE,
        BRACE_OPEN,
        BRACE_CLOSE,

        /*
         * Register access sigils.
         */
        RA_VOID,
        RA_DIRECT,
        RA_PTR_DEREF,

        /*
         * Literals.
         */
        LITERAL_STRING,
        LITERAL_INTEGER,
        LITERAL_FLOAT,
        LITERAL_ATOM,

        /*
         * Definitions.
         */
        DEF_FUNCTION,
        DEF_BLOCK,
        END,

        /*
         * Fluff.
         */
        WHITESPACE,
        COMMENT,
    };
    auto to_string(TOKEN const) -> std::string;

    struct Lexeme {
        std::string text;
        TOKEN token;
        Location location;

        auto operator==(TOKEN const) const -> bool;
        auto operator==(std::string_view const) const -> bool;
    };

    auto lex(std::string_view) -> std::vector<Lexeme>;
}

#endif
