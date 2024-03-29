/*
 *  Copyright (C) 2018, 2021-2022 Marek Marecki
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

#ifndef VIUA_LIBS_ERRORS_COMPILE_TIME_H
#define VIUA_LIBS_ERRORS_COMPILE_TIME_H

#include <string>
#include <string_view>
#include <utility>

#include <viua/libs/lexer.h>


namespace viua::libs::errors::compile_time {
enum class Cause {
    /*
     * Unknown cause is for errors which don't have a precise cause, or used to
     * initialise Cause variables.
     */
    Unknown = 0,

    /*
     * None is for errors which do not represent errors per-se, or for errors
     * which require custom descriptions.
     */
    None,

    /*
     * Encountered character can not form a legal token. This error is only
     * thrown during lexical analysis.
     */
    Illegal_character,

    /*
     * Encountered token is either invalid (maybe malformed?), or just
     * unexpected at its current position.
     *
     * Invalid tokens are NEVER valid.
     * Unexpected ones are otherwise valid, just not at the current position.
     */
    Invalid_token,
    Unexpected_token,

    Unknown_directive,
    Unknown_opcode,
    Invalid_register_access,
    Too_few_operands,

    Reference_to_undefined_symbol,
    Call_to_undefined_function,
    Jump_to_undefined_label,

    Invalid_reference,

    Value_out_of_range,
    Invalid_operand,

    Duplicated_entry_point,

    Unknown_type,
    Unknown_label,

    Invalid_cast,
};
auto to_string(Cause const) -> std::string_view;

struct Error {
    /*
     * Basic description of the error consists of an enumerated cause (to make
     * describing and looking for errors easier), and a custom-string message
     * detailing the exact circumstances of the error.
     *
     * The message may be empty if the error cause is self-explanatory when
     * taken together with the highlighted lexemes.
     */
    Cause const cause{Cause::Unknown};
    std::string message;

    /*
     * The main lexeme is the main location at which the error is reported, and
     * should be the most important lexeme from the highlighted ones.
     *
     * Additional lexemes are used to highlight details about the error, or
     * other tokens that should be looked at (if appropriate). There may be no
     * detail lexemes, as they are not always useful.
     *
     * Please note that details MAY NOT necessarily be on THE SAME LINE as the
     * main lexeme.
     */
    using Lexeme = viua::libs::lexer::Lexeme;
    Lexeme const main_lexeme;
    std::vector<std::pair<Lexeme, bool>> detail_lexemes;

    /*
     * Notes are little messages displayed below the main error message,
     * providing additional details, suggestions, or results of analysis
     * performed by the assembler. They are dependent on the PRECISE cause of
     * the error, eg, providing a candidate for a misspelling, or report of
     * range overflow.
     *
     * They are displayed like this:
     *
     *      example.asm:42:8: error: instruction would frobnicate the process
     *      example.asm:42:8: note: did you mean to bamboozle the process?
     *
     * Their location is the same as that of the main lexeme.
     */
    std::vector<std::string> attached_notes;

    /*
     * Comments are messages providing general guidance about the error. Instead
     * of being based on the PRECISE circumstances (like the notes are), they
     * are based on the GENERAL cause of the error. They may provide links to
     * assembly language or VM documentation, inform about general properties of
     * types, etc.
     *
     * They are displayed like this:
     *
     *      example.asm:42:8: error: instruction would bamboozle the process
     *      example.asm:42:8: comment: formal theory of bamboozling is \
     *          described by https://example.com/bamboozle
     *
     * Their location is the same as that of the main lexeme.
     */
    std::vector<std::string> attached_comments;

    /*
     * An aside note is something immediately useful and important, and what
     * should be displayed prominently attached to a specific lexeme. It may be
     * a restatement of a note with a more precise location.
     *
     * It is displayed like this (if connected with the main error):
     *
     *     41 |     lui $1, 0xdeadbeef
     *     42 |     bamboozle $1, "Ha!"
     *        |     ^~~~~~~~~
     *        .     |
     *        .     ` this would bamboozle the VM
     *        .
     *     43 |     addi $1, void, -1
     *
     * ...or like this (if not connected with the main error):
     *
     *     41 |     lui $1, 0xdeadbeef
     *        .             ^~~~~~~~~~
     *        .             |
     *        .             ` this would bamboozle the VM
     *        .
     *     42 |     bamboozle $1, "Ha!"
     *        |     ^~~~~~~~~
     *        |
     *     43 |     addi $1, void, -1
     *
     * Its location is independent of the main lexeme.
     *
     * FIXME Only one aside note is possible.
     * FIXME Aside notes can only be connected to the main lexeme.
     */
    std::string aside_note;
    std::optional<viua::libs::lexer::Lexeme> aside_lexeme;

    /*
     * Sometimes errors must be chained, because error A causes error B, which
     * in turns causes error C, and so on.
     */
    std::vector<Error> fallout;

    Error(viua::libs::lexer::Lexeme, Cause const, std::string = "");

    auto chain(Error&&) -> Error&;

    auto aside(std::string, std::optional<Lexeme> = std::nullopt) -> Error&;
    auto aside() const -> std::string_view;
    auto aside_character() const -> size_t
    {
        if (aside_lexeme.has_value()) {
            return aside_lexeme->location.character;
        }
        return character();
    }

    auto note(std::string) & -> Error&;
    auto note(std::string) && -> Error;
    auto notes() const -> std::vector<std::string> const&;

    auto add(Lexeme, bool const = false) & -> Error&;
    auto add(Lexeme, bool const = false) && -> Error;
    using span_type =
        std::tuple<std::string::size_type, std::string::size_type, bool>;
    auto spans() const -> std::vector<span_type>;

    /*
     * Location of the main point of error.
     */
    inline auto main() const -> Lexeme
    {
        return main_lexeme;
    }
    inline auto location() const -> viua::libs::lexer::Location
    {
        return main().location;
    }
    inline auto line() const -> size_t
    {
        return location().line;
    }
    inline auto character() const -> size_t
    {
        return location().character;
    }
    inline auto offset() const -> size_t
    {
        return location().offset;
    }

    /*
     * Error message formatting.
     */
    auto what() const -> std::string_view;
    auto str() const -> std::string;
};
}  // namespace viua::libs::errors::compile_time


#endif
