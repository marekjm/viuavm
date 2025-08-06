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

enum class TOKEN
{
    /*
     * Used for synthetic errors during lexical analysis phase, when there are
     * no tokens available yet.
     */
    INVALID,

    /*
     * Fluff and noise.
     */
    WHITESPACE,
    COMMENT,

    /*
     * Assembler directives.
     */
    SWITCH_TO_TEXT,
    SWITCH_TO_RODATA,
    SWITCH_TO_SECTION,

    DECLARE_SYMBOL,
    DEFINE_LABEL,
    BEGIN,
    END,
    ALLOCATE_OBJECT,

    /*
     * Instructions.
     */
    OPCODE,

    VOID,

    /*
     * Literals.
     */
    LITERAL_ATOM,
    LITERAL_INTEGER,
    LITERAL_FLOAT,
    LITERAL_STRING,

    /*
     * Control characters.
     */
    COMMA,
    ELLIPSIS,
    DOT,
    EQ,
    AT,
    DOLLAR,
    STAR,
    TERMINATOR,
    ATTR_LIST_OPEN,
    ATTR_LIST_CLOSE,
};
auto to_string(TOKEN const) -> std::string;

struct Lexeme {
    std::string text;
    TOKEN token;
    Location location;

    std::optional<std::tuple<std::string, TOKEN, Location>> synthesized_from{};

    Lexeme() = default;
    inline Lexeme(
        std::string tx,
        TOKEN tk,
        Location ln)
        : text{ std::move(tx) }
        , token{ tk }
        , location{ ln }
    {}

    auto operator==(TOKEN const) const -> bool;
    auto operator==(std::string_view const) const -> bool;

    auto make_synth(std::string, TOKEN const) const -> Lexeme;
    inline auto make_synth() const -> Lexeme
    {
        return make_synth(text, token);
    }
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
inline auto const OPCODE_NAMES = std::set<std::string_view>{
    "noop",
    "halt",
    "ebreak",
    "ecall",
    "return",

    "add",
    "sub",
    "mul",
    "div",
    "mod",

    "bitshl",
    "bitshr",
    "bitashr",
    "bitrol",
    "bitror",
    "bitand",
    "bitor",
    "bitxor",
    "bitnot",

    "eq",
    "lt",
    "gt",
    "cmp",

    "and",
    "or",
    "not",

    "call",

    "atom",

    "lui",
    "luiu",
    "lli",
    "float",
    "double",

    "addi",
    "subi",
    "muli",
    "divi",

    "frame",
    "tailcall",

    "copy",
    "move",
    "swap",

    "if",

    "io_submit",
    "io_wait",
    "io_shutdown",
    "io_ctl",
    "io_peek",

    "actor",
    "self",

    "gts",
    "gtl",

    "cast",

    "arodp",
    "atxtp",

    "sm",
    "lm",
    "ama",
    "amd",
    "ptr",

    /*
     * Pseudoinstructions listed below.
     */
    "li",
    "delete",
    "jump",

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

namespace pattern {
constexpr auto LITERAL_ATOM = "^[A-Za-z_][A-Za-z0-9_]*\\b";
}

auto lex(std::string_view) -> std::vector<Lexeme>;

namespace stage {
auto lex(std::filesystem::path const, std::string_view const)
    -> std::vector<Lexeme>;

auto remove_noise(std::vector<Lexeme>&&) -> std::vector<Lexeme>;
}  // namespace stage
}  // namespace viua::libs::lexer

#endif
