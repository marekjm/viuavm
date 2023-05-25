/*
 *  Copyright (C) 2021-2022 Marek Marecki
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

#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>


namespace viua::libs::lexer {
struct Location {
    size_t line{};
    size_t character{};
    size_t offset{};
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
    DOT,
    EQ,
    AT,
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
    DEF_LABEL,
    DEF_VALUE,
    END,

    /*
     * Fluff.
     */
    WHITESPACE,
    COMMENT,

    /*
     * Used for synthetic errors during lexical analysis phase, when there are
     * no tokens available yet.
     */
    INVALID,
};
auto to_string(TOKEN const) -> std::string;

struct Lexeme {
    std::string text;
    TOKEN token;
    Location location;

    std::optional<std::tuple<std::string, TOKEN, Location>> synthesized_from{};

    Lexeme() = default;
    inline Lexeme(std::string tx, TOKEN tk, Location ln)
        : text{std::move(tx)}
        , token{tk}
        , location{ln}
    {}

    auto operator==(TOKEN const) const -> bool;
    auto operator==(std::string_view const) const -> bool;

    auto make_synth(std::string, TOKEN const) const -> Lexeme;
    auto is_synth() const -> bool;
    auto synthed_from() const -> Lexeme;
};

/*
 * The "inline" specifier is used to avoid multiple definition errors.
 * Usually, things should only be declared, and not defined in the header
 * files because the linker will probably see multiple definitions of such
 * defined symbol and refuse to link the object code.
 *
 * However, with the "inline" specifier in place we assure the linker that
 * all definitions are the same so it can choose whichever it likes (eg, the
 * first one it found).
 *
 * With this one trick we can now feel free to define variables in header
 * files. Neat!
 */
inline auto const OPCODE_NAMES = std::set<std::string>{
    "noop",
    "halt",
    "ebreak",
    "ecall",
    "return",

    "add",
    "g.add",
    "sub",
    "g.sub",
    "mul",
    "g.mul",
    "div",
    "g.div",
    "mod",
    "g.mod",

    "bitshl",
    "g.bitshl",
    "bitshr",
    "g.bitshr",
    "bitashr",
    "g.bitashr",
    "bitrol",
    "g.bitrol",
    "bitror",
    "g.bitror",
    "bitand",
    "g.bitand",
    "bitor",
    "g.bitor",
    "bitxor",
    "g.bitxor",
    "bitnot",
    "g.bitnot",

    "eq",
    "g.eq",
    "lt",
    "g.lt",
    "gt",
    "g.gt",
    "cmp",
    "g.cmp",

    "and",
    "g.and",
    "or",
    "g.or",
    "not",
    "g.not",

    "call",

    "atom",
    "g.atom",
    "string",
    "g.string",
    "struct",
    "g.struct",
    "buffer",
    "g.buffer",

    "lui",
    "g.lui",
    "luiu",
    "g.luiu",
    "float",
    "g.float",
    "double",
    "g.double",

    "addi",
    "g.addi",
    "subi",
    "g.subi",
    "muli",
    "g.muli",
    "divi",
    "g.divi",

    "frame",
    "g.frame",
    "tailcall",

    "copy",
    "g.copy",
    "move",
    "g.move",
    "swap",
    "g.swap",

    "buffer_push",
    "g.buffer_push",
    "buffer_size",
    "g.buffer_size",
    "buffer_at",
    "g.buffer_at",
    "buffer_pop",
    "g.buffer_pop",

    "ref",
    "g.ref",

    "struct_at",
    "g.struct_at",
    "struct_insert",
    "g.struct_insert",
    "struct_remove",
    "g.struct_remove",

    "if",
    "g.if",

    "g.io_submit",
    "io_submit",
    "g.io_wait",
    "io_wait",
    "g.io_shutdown",
    "io_shutdown",
    "g.io_ctl",
    "io_ctl",
    "g.io_peek",
    "io_peek",

    "actor",
    "g.actor",
    "self",
    "g.self",

    "sm",
    "g.sm",
    "lm",
    "g.lm",
    "ama",
    "g.ama",
    "amd",
    "g.amd",
    "ptr",
    "g.ptr",

    /*
     * Pseudoinstructions listed below.
     */
    "li",
    "g.li",
    "delete",
    "g.delete",
    "jump",
    "g.jump",

    "sb",
    "lb",
    "mb",
    "sh",
    "lh",
    "mh",
    "sw",
    "lw",
    "mw",
    "sd",
    "ld",
    "md",
    "sq",
    "lq",
    "mq",

    "amba",
    "amha",
    "amwa",
    "amda",
    "amqa",
    "ambd",
    "amhd",
    "amwd",
    "amdd",
    "amqd",
};

auto lex(std::string_view) -> std::vector<Lexeme>;

namespace stage {
auto lexical_analysis(std::filesystem::path const, std::string_view const)
    -> std::vector<Lexeme>;
}
}  // namespace viua::libs::lexer

#endif
