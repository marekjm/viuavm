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

#include <assert.h>
#include <elf.h>
#include <endian.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <experimental/memory>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <viua/arch/arch.h>
#include <viua/arch/elf.h>
#include <viua/arch/ops.h>
#include <viua/libs/assembler.h>
#include <viua/libs/errors/compile_time.h>
#include <viua/libs/lexer.h>
#include <viua/libs/stage.h>
#include <viua/support/number.h>
#include <viua/support/string.h>
#include <viua/support/tty.h>
#include <viua/support/errno.h>
#include <viua/support/vector.h>


constexpr auto DEBUG_LEX       = true;
constexpr auto DEBUG_PARSE     = true;
constexpr auto DEBUG_EXPANSION = false;

using viua::libs::stage::save_buffer_to_rodata;
using viua::libs::stage::save_string_to_strtab;
using viua::support::string::quote_fancy;

#if 0
namespace {
using namespace viua::libs::parser;
auto emit_bytecode(std::vector<std::unique_ptr<ast::Node>> const& nodes,
                   std::vector<viua::arch::instruction_type>& text,
                   std::vector<Elf64_Sym>& symbol_table,
                   std::map<std::string, size_t> const& symbol_map)
    -> std::map<std::string, uint64_t>
{
    auto const ops_count =
        1
        + std::accumulate(
            nodes.begin(),
            nodes.end(),
            size_t{0},
            [](size_t const acc,
               std::unique_ptr<ast::Node> const& each) -> size_t {
                if (dynamic_cast<ast::Fn_def*>(each.get()) == nullptr) {
                    return acc;
                }

                auto& fn = static_cast<ast::Fn_def&>(*each);
                return (acc + fn.instructions.size());
            });

    {
        text.reserve(ops_count);
        text.resize(ops_count);

        using viua::arch::instruction_type;
        using viua::arch::ops::N;
        using viua::arch::ops::OPCODE;
        text.at(0) = N{static_cast<instruction_type>(OPCODE::HALT)}.encode();
    }

    auto fn_addresses = std::map<std::string, uint64_t>{};
    auto ip           = (text.data() + 1);
    for (auto const& each : nodes) {
        if (dynamic_cast<ast::Fn_def*>(each.get()) == nullptr) {
            continue;
        }

        auto& fn = static_cast<ast::Fn_def&>(*each);
        {
            /*
             * Save the function's address (offset into the .text section,
             * really) in the functions table. This is needed not only for
             * debugging, but also because the functions' addresses are resolved
             * dynamically for call and similar instructions. Why dynamically?
             * FIXME The above is no longer true.
             *
             * Because there is a strong distinction between calls to bytecode
             * and foreign functions. At compile time, we don't yet know,
             * though, which function is foreign and which is bytecode.
             */
            auto const fn_addr =
                (ip - &text[0]) * sizeof(viua::arch::instruction_type);
            auto& sym = symbol_table.at(symbol_map.at(fn.name.text));

            if (not fn.has_attr("extern")) {
                sym.st_value = fn_addr;
                sym.st_size  = fn.instructions.size()
                              * sizeof(viua::arch::instruction_type);
            }
        }

        for (auto const& insn : fn.instructions) {
            *ip++ = viua::libs::stage::emit_instruction(insn);
        }
    }

    return fn_addresses;
}
}  // namespace

namespace stage {
using Lexemes   = std::vector<viua::libs::lexer::Lexeme>;
using AST_nodes = std::vector<std::unique_ptr<ast::Node>>;
auto syntactical_analysis(std::filesystem::path const source_path,
                          std::string_view const source_text,
                          Lexemes const& lexemes) -> AST_nodes
{
    try {
        return parse(lexemes);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        viua::libs::stage::display_error_and_exit(source_path, source_text, e);
    }
}

auto load_value_labels(std::filesystem::path const source_path,
                       std::string_view const source_text,
                       AST_nodes const& nodes,
                       std::vector<uint8_t>& rodata_buf,
                       std::vector<uint8_t>& string_table,
                       std::vector<Elf64_Sym>& symbol_table,
                       std::map<std::string, size_t>& symbol_map) -> void
{
    for (auto const& each : nodes) {
        if (dynamic_cast<ast::Label_def*>(each.get()) == nullptr) {
            continue;
        }

        auto& ct = static_cast<ast::Label_def&>(*each);
        if (ct.has_attr("extern")) {
            auto const name_off =
                save_string_to_strtab(string_table, ct.name.text);

            auto symbol     = Elf64_Sym{};
            symbol.st_name  = name_off;
            symbol.st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_OBJECT);
            symbol.st_other = STV_DEFAULT;
            viua::libs::stage::record_symbol(
                ct.name.text, symbol, symbol_table, symbol_map);

            /*
             * Neither address nor size of the extern symbol is known, only its
             * label.
             */
            symbol.st_value = 0;
            symbol.st_size  = 0;
            return;
        }

        if (ct.type == "string") {
            auto s = std::string{};
            for (auto i = size_t{0}; i < ct.value.size(); ++i) {
                auto& each = ct.value.at(i);

                using enum viua::libs::lexer::TOKEN;
                if (each.token == LITERAL_STRING) {
                    auto tmp = each.text;
                    tmp      = tmp.substr(1, tmp.size() - 2);
                    tmp      = viua::support::string::unescape(tmp);
                    s += tmp;
                } else if (each.token == STAR) {
                    auto& next = ct.value.at(++i);
                    if (next.token != LITERAL_INTEGER) {
                        using viua::libs::errors::compile_time::Cause;
                        using viua::libs::errors::compile_time::Error;

                        auto const e = Error{each,
                                             Cause::Invalid_operand,
                                             "cannot multiply string constant "
                                             "by non-integer"}
                                           .add(next)
                                           .add(ct.value.at(i - 2))
                                           .aside("right-hand side must be an "
                                                  "positive integer");
                        viua::libs::stage::display_error_and_exit(
                            source_path, source_text, e);
                    }

                    auto x = ston<size_t>(next.text);
                    auto o = std::ostringstream{};
                    for (auto i = size_t{0}; i < x; ++i) {
                        o << s;
                    }
                    s = o.str();
                }
            }

            auto const value_off = save_buffer_to_rodata(rodata_buf, s);
            auto const name_off =
                save_string_to_strtab(string_table, ct.name.text);

            auto symbol     = Elf64_Sym{};
            symbol.st_name  = name_off;
            symbol.st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_OBJECT);
            symbol.st_other = STV_DEFAULT;

            symbol.st_value = value_off;
            symbol.st_size  = s.size();

            /*
             * Section header table index (see elf(5) for st_shndx) is filled
             * out later, since at this point we do not have this information.
             *
             * For variables it will be the index of .rodata.
             * For functions it will be the index of .text.
             */
            symbol.st_shndx = 0;

            viua::libs::stage::record_symbol(
                ct.name.text, symbol, symbol_table, symbol_map);
        } else if (ct.type == "atom") {
            auto const s = ct.value.front().text;

            auto const value_off = save_buffer_to_rodata(rodata_buf, s);
            auto const name_off =
                save_string_to_strtab(string_table, ct.name.text);

            auto symbol     = Elf64_Sym{};
            symbol.st_name  = name_off;
            symbol.st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_OBJECT);
            symbol.st_other = STV_DEFAULT;

            symbol.st_value = value_off;
            symbol.st_size  = s.size();

            /*
             * Section header table index (see elf(5) for st_shndx) is filled
             * out later, since at this point we do not have this information.
             *
             * For variables it will be the index of .rodata.
             * For functions it will be the index of .text.
             */
            symbol.st_shndx = 0;

            viua::libs::stage::record_symbol(
                ct.name.text, symbol, symbol_table, symbol_map);
        }
    }
}

auto load_function_labels(AST_nodes const& nodes,
                          std::vector<uint8_t>& string_table,
                          std::vector<Elf64_Sym>& symbol_table,
                          std::map<std::string, size_t>& symbol_map) -> void
{
    for (auto const& each : nodes) {
        if (dynamic_cast<ast::Fn_def*>(each.get()) == nullptr) {
            continue;
        }

        auto const& fn = static_cast<ast::Fn_def&>(*each);

        auto const name_off = save_string_to_strtab(string_table, fn.name.text);

        auto symbol     = Elf64_Sym{};
        symbol.st_name  = name_off;
        symbol.st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_FUNC);
        symbol.st_other = STV_DEFAULT;

        /*
         * Leave size and offset of the function empty since we do not have this
         * information yet. It will only be available after the bytecode is
         * emitted.
         *
         * For functions marked as [[extern]] st_value will be LEFT EMPTY after
         * the assembler exits, as a signal to the linker that this symbol was
         * defined in a different module and needs to be resolved.
         */
        symbol.st_size  = 0;
        symbol.st_value = 0;

        /*
         * Section header table index (see elf(5) for st_shndx) is filled
         * out later, since at this point we do not have this information.
         *
         * For variables it will be the index of .rodata.
         * For functions it will be the index of .text.
         */
        symbol.st_shndx = 0;

        viua::libs::stage::record_symbol(
            fn.name.text, symbol, symbol_table, symbol_map);
    }
}

auto cook_long_immediates(std::filesystem::path const source_path,
                          std::string_view const source_text,
                          AST_nodes const& nodes,
                          std::vector<uint8_t>& rodata_buf,
                          std::vector<Elf64_Sym>& symbol_table,
                          std::map<std::string, size_t>& symbol_map) -> void
{
    for (auto const& each : nodes) {
        if (dynamic_cast<ast::Fn_def*>(each.get()) == nullptr) {
            continue;
        }

        auto& fn = static_cast<ast::Fn_def&>(*each);

        auto cooked = std::vector<ast::Instruction>{};
        for (auto& insn : fn.instructions) {
            try {
                auto c = viua::libs::stage::cook_long_immediates(
                    insn, rodata_buf, symbol_table, symbol_map);
                std::copy(c.begin(), c.end(), std::back_inserter(cooked));
            } catch (viua::libs::errors::compile_time::Error const& e) {
                viua::libs::stage::display_error_in_function(
                    source_path, e, fn.name.text);
                viua::libs::stage::display_error_and_exit(
                    source_path, source_text, e);
            }
        }
        fn.instructions = std::move(cooked);
    }
}

auto cook_pseudoinstructions(std::filesystem::path const source_path,
                             std::string_view const source_text,
                             AST_nodes& nodes,
                             std::map<std::string, size_t> const& symbol_map)
    -> void
{
    for (auto const& each : nodes) {
        if (dynamic_cast<ast::Fn_def*>(each.get()) == nullptr) {
            continue;
        }

        auto& fn                 = static_cast<ast::Fn_def&>(*each);
        auto const raw_ops_count = fn.instructions.size();
        try {
            fn.instructions = viua::libs::stage::expand_pseudoinstructions(
                std::move(fn.instructions), symbol_map);
        } catch (viua::libs::errors::compile_time::Error const& e) {
            viua::libs::stage::display_error_in_function(
                source_path, e, fn.name.text);
            viua::libs::stage::display_error_and_exit(
                source_path, source_text, e);
        }

        if constexpr (DEBUG_EXPANSION) {
            std::cerr << "FN " << fn.to_string() << " with " << raw_ops_count
                      << " raw, " << fn.instructions.size() << " baked op(s)\n";
            auto physical_index = size_t{0};
            for (auto const& op : fn.instructions) {
                std::cerr << "  " << std::setw(4) << std::setfill('0')
                          << std::hex << physical_index++ << " " << std::setw(4)
                          << std::setfill('0') << std::hex << op.physical_index
                          << "  " << op.to_string() << "\n";
            }
        }
    }
}

using Text         = std::vector<viua::arch::instruction_type>;
using Fn_addresses = std::map<std::string, uint64_t>;
auto emit_bytecode(std::filesystem::path const source_path,
                   std::string_view const source_text,
                   AST_nodes const& nodes,
                   std::vector<Elf64_Sym>& symbol_table,
                   std::map<std::string, size_t> const& symbol_map) -> Text
{
    /*
     * Calculate function spans in source code for error reporting. This way an
     * error offset can be matched to a function without the error having to
     * carry the function name.
     */
    auto fn_spans =
        std::vector<std::pair<std::string, std::pair<size_t, size_t>>>{};
    for (auto const& each : nodes) {
        if (dynamic_cast<ast::Fn_def*>(each.get()) == nullptr) {
            continue;
        }

        auto& fn = static_cast<ast::Fn_def&>(*each);
        fn_spans.emplace_back(
            fn.name.text,
            std::pair<size_t, size_t>{fn.start.location.offset,
                                      fn.end.location.offset});
    }

    auto text         = std::vector<viua::arch::instruction_type>{};
    auto fn_addresses = std::map<std::string, uint64_t>{};
    try {
        fn_addresses = ::emit_bytecode(nodes, text, symbol_table, symbol_map);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        auto fn_name = std::optional<std::string>{};

        for (auto const& [name, offs] : fn_spans) {
            auto const [low, high] = offs;
            auto const off         = e.location().offset;
            if ((off >= low) and (off <= high)) {
                fn_name = name;
            }
        }

        if (fn_name.has_value()) {
            viua::libs::stage::display_error_in_function(
                source_path, e, *fn_name);
        }
        viua::libs::stage::display_error_and_exit(source_path, source_text, e);
    }

    return text;
}
}  // namespace stage
#endif

namespace {
using viua::libs::lexer::Lexeme;
auto dump_lexemes(std::vector<Lexeme> const& lexemes) -> void
{
    auto const max_line = lexemes.back().location.line;
    auto const pad_line = std::to_string(max_line).size();

    auto const max_char =
        std::max_element(lexemes.begin(),
                         lexemes.end(),
                         [](Lexeme const& lhs, Lexeme const& rhs) -> bool {
                             return lhs.location.character
                                    < rhs.location.character;
                         })
            ->location.character;
    auto const max_size =
        std::max_element(lexemes.begin(),
                         lexemes.end(),
                         [](Lexeme const& lhs, Lexeme const& rhs) -> bool {
                             return lhs.text.size() < rhs.text.size();
                         })
            ->text.size();
    auto const pad_location =
        1 + std::to_string(max_char).size() + std::to_string(max_size).size();

    auto const max_offset = lexemes.back().location.offset;
    auto const pad_offset = std::to_string(max_offset).size();

    for (auto const& each : lexemes) {
        auto token_kind = viua::libs::lexer::to_string(each.token);
        token_kind.resize(17, ' ');

        auto loc =
            std::to_string(each.location.character) + '-'
            + std::to_string(each.location.character + each.text.size() - 1);
        loc.resize(pad_location, ' ');

        auto off = "+" + std::to_string(each.location.offset);
        off.resize(2 + pad_offset, ' ');

        std::cerr << "  " << token_kind;
        std::cerr << "  " << std::setw(pad_line) << each.location.line << ':'
                  << loc;
        std::cerr << "  " << off;

        using viua::libs::lexer::TOKEN;
        auto const printable = (each.token == TOKEN::LITERAL_STRING)
                               or (each.token == TOKEN::LITERAL_INTEGER)
                               or (each.token == TOKEN::LITERAL_FLOAT)
                               or (each.token == TOKEN::LITERAL_ATOM)
                               or (each.token == TOKEN::OPCODE);
        if (printable) {
            std::cerr << "  " << each.text;
        }

        std::cerr << "\n";
    }
}

auto looks_octal(std::string_view const sv) -> bool
{
    return std::all_of(sv.begin(), sv.end(), [](char const c) -> bool {
        return (c >= '0') and (c <= '7');
    });
}
auto any_find_mistake(std::vector<Lexeme> const& lexemes)
    -> std::optional<viua::libs::errors::compile_time::Error>
{
    if (lexemes.empty()) {
        return std::nullopt;
    }

    using viua::libs::lexer::TOKEN;

    auto const at = [&lexemes](size_t const n) -> Lexeme const& {
        return lexemes.at(n);
    };
    auto const is = [&lexemes](size_t const n, TOKEN const t) -> bool {
        return lexemes.at(n).token == t;
    };
    auto const iso =
        [&lexemes](size_t const n, ssize_t const off, TOKEN const t) -> bool {
        if ((off < 0) and (static_cast<size_t>(std::abs(off)) > n)) {
            return false;
        }
        auto const effective_n = n + off;
        return lexemes.at(effective_n).token == t;
    };
    auto const next_is_glued = [&lexemes](size_t const n) -> bool {
        return (lexemes.at(n).location.offset + lexemes.at(n).text.size())
               == lexemes.at(n + 1).location.offset;
    };

    /*
     * Detect invalid numeric literals.
     */
    for (auto i = size_t{0}; i < (lexemes.size() - 1); ++i) {
        if (not next_is_glued(i)) {
            continue;
        }
        if (is(i + 1, TOKEN::TERMINATOR)) {
            continue;
        }

        constexpr auto NOT_A_VALID_NUMERIC_LITERAL =
            "not a valid numeric literal";
        if (is(i, TOKEN::LITERAL_INTEGER)
            and is(i + 1, TOKEN::LITERAL_INTEGER)) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            if (at(i).text == "0" and looks_octal(at(i + 1).text)) {
                auto e = Error{
                    at(i), Cause::Invalid_token, NOT_A_VALID_NUMERIC_LITERAL};
                e.add(at(i + 1));
                e.aside("use " + quote_fancy("0o")
                        + " prefix for octal literals");
                return e;
            }

            auto synth_text = at(i).text + at(i + 1).text;
            auto synth      = at(i).make_synth(synth_text, at(i).token);
            return Error{
                synth, Cause::Invalid_token, NOT_A_VALID_NUMERIC_LITERAL};
        }
        if (is(i, TOKEN::LITERAL_INTEGER)
            and not is(i + 1, TOKEN::LITERAL_INTEGER)) {
            /*
             * Do not raise the alarm if the numeral has a glued lexeme as part
             * of a register index. Parsing register indexes is a better place
             * to report errors in this case.
             *
             * Or if it the glued character is a comma.
             */
            auto const yeah_looks_ok = iso(i, -1, TOKEN::DOLLAR)
                                       or iso(i, +1, TOKEN::COMMA);
            if (not yeah_looks_ok) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;

                auto e = Error{at(i + 1),
                               Cause::Invalid_token,
                               NOT_A_VALID_NUMERIC_LITERAL};
                e.add(at(i));
                return e;
            }
        }
    }

    return std::nullopt;
}
}  // namespace

namespace {
namespace ast {
struct Node {
    using Lexeme         = viua::libs::lexer::Lexeme;
    using attribute_type = std::pair<Lexeme, std::optional<Lexeme>>;
    std::vector<attribute_type> attributes;

    Lexeme leader;

    auto has_attr(std::string_view const) const
        -> std::optional<attribute_type>;
    auto attr [[maybe_unused]] (std::string_view const) const
        -> std::optional<Lexeme>;
};
auto Node::has_attr(std::string_view const key) const
    -> std::optional<attribute_type>
{
    for (auto const& each : attributes) {
        if (each.first == key) {
            return each;
        }
    }
    return {};
}
auto Node::attr(std::string_view const key) const -> std::optional<Lexeme>
{
    for (auto const& each : attributes) {
        if (each.first == key) {
            return each.second;
        }
    }
    return {};
}

/*
 * Node = Section String
 *      | Label Attrs? id
 *      | Symbol Attrs? id
 *      | Define_object Attrs? Type Value*
 *      | Opcode id Operands?
 *
 * Value = String
 *       | Int
 *       | Float
 *       | Atom
 *
 * Operands = Operand ("," Operand)*
 *
 * Operand = Attrs? Immediate | Register
 *
 * Attrs = "[[" Attr ("," Attr)* "]]"
 *
 * Attr = id ("=" Value)?
 */
struct Section : Node {
    Lexeme name;

    auto which() const -> std::string_view;
};
auto Section::which() const -> std::string_view
{
    return std::string_view{name.text.c_str() + 1, name.text.size() - 2};
}
struct Label : Node {
    Lexeme name;
};
struct Symbol : Node {
    Lexeme name;
};
struct Object : Node {
    Lexeme type;
    std::vector<Lexeme> ctor;
};
struct Operand : Node {
    std::vector<Lexeme> ingredients;
};
struct Instruction : Node {
    std::vector<Operand> operands;
};
struct Begin : Node {};
struct End : Node {};
}  // namespace ast
auto dump_nodes(std::vector<std::unique_ptr<ast::Node>> const& nodes) -> void
{
    auto const max_line = nodes.back()->leader.location.line;
    auto const pad_line = std::to_string(max_line).size();

    auto const max_char =
        std::max_element(nodes.begin(),
                         nodes.end(),
                         [](auto const& lhs, auto const& rhs) -> bool {
                             return lhs->leader.location.character
                                    < rhs->leader.location.character;
                         })
            ->get()
            ->leader.location.character;
    auto const max_size =
        std::max_element(nodes.begin(),
                         nodes.end(),
                         [](auto const& lhs, auto const& rhs) -> bool {
                             return lhs->leader.text.size()
                                    < rhs->leader.text.size();
                         })
            ->get()
            ->leader.text.size();
    auto const pad_location =
        1 + std::to_string(max_char).size() + std::to_string(max_size).size();

    auto const max_offset = nodes.back()->leader.location.offset;
    auto const pad_offset = std::to_string(max_offset).size();

    for (auto const& each : nodes) {
        auto const& leader = each->leader;
        auto token_kind    = viua::libs::lexer::to_string(leader.token);
        token_kind.resize(17, ' ');

        auto loc = std::to_string(leader.location.character) + '-'
                   + std::to_string(leader.location.character
                                    + leader.text.size() - 1);
        loc.resize(pad_location, ' ');

        auto off = "+" + std::to_string(leader.location.offset);
        off.resize(2 + pad_offset, ' ');

        std::cerr << "  " << token_kind;
        std::cerr << "  " << std::setw(pad_line) << leader.location.line << ':'
                  << loc;
        std::cerr << "  " << off;

        auto printable = std::string{};
        switch (leader.token) {
            using enum viua::libs::lexer::TOKEN;
        case SWITCH_TO_SECTION:
            printable = static_cast<ast::Section&>(*each).which();
            break;
        case DEFINE_LABEL:
            printable = static_cast<ast::Label&>(*each).name.text;
            break;
        case DECLARE_SYMBOL:
            printable = static_cast<ast::Symbol&>(*each).name.text;
            break;
        case ALLOCATE_OBJECT:
            printable = static_cast<ast::Object&>(*each).type.text;
            break;
        case OPCODE:
            printable = static_cast<ast::Instruction&>(*each).leader.text;
            break;
        default:
            break;
        }
        if (not printable.empty()) {
            std::cerr << "  " << printable;
        }

        std::cerr << "\n";
    }
}

auto did_you_mean(viua::libs::errors::compile_time::Error& e, std::string what)
    -> viua::libs::errors::compile_time::Error&
{
    return e.aside("did you mean \"" + what + "\"?");
}
auto did_you_mean(viua::libs::errors::compile_time::Error&& e, std::string what)
    -> viua::libs::errors::compile_time::Error
{
    did_you_mean(e, what);
    return e;
}

auto consume_token_of(
    viua::libs::lexer::TOKEN const tt,
    viua::support::vector_view<viua::libs::lexer::Lexeme>& lexemes)
    -> viua::libs::lexer::Lexeme
{
    if (lexemes.front().token != tt) {
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        auto e = Error{lexemes.front(), Cause::Unexpected_token};
        {
            constexpr auto esc = viua::support::tty::send_escape_seq;
            constexpr auto q   = viua::support::string::quote_fancy;
            using viua::support::tty::ATTR_FONT_BOLD;
            using viua::support::tty::ATTR_FONT_NORMAL;

            auto const BOLD = std::string{esc(2, ATTR_FONT_BOLD)};
            auto const NORM = std::string{esc(2, ATTR_FONT_NORMAL)};

            e.aside("expected "
                    + q(BOLD + viua::libs::lexer::to_string(tt) + NORM));
        }
        throw e;
    }
    auto lx = lexemes.front();
    lexemes.remove_prefix(1);
    return lx;
}
auto consume_token_of(
    std::set<viua::libs::lexer::TOKEN> const ts,
    viua::support::vector_view<viua::libs::lexer::Lexeme>& lexemes)
    -> viua::libs::lexer::Lexeme
{
    if (ts.count(lexemes.front().token) == 0) {
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        auto e = Error{lexemes.front(), Cause::Unexpected_token};
        {
            auto s = std::ostringstream{};
            s << "expected ";
            if (ts.size() > 1) {
                s << "one of: ";
            }

            constexpr auto esc = viua::support::tty::send_escape_seq;
            constexpr auto q   = viua::support::string::quote_fancy;
            using viua::support::tty::ATTR_FONT_BOLD;
            using viua::support::tty::ATTR_FONT_NORMAL;

            auto const BOLD = std::string{esc(2, ATTR_FONT_BOLD)};
            auto const NORM = std::string{esc(2, ATTR_FONT_NORMAL)};

            auto it = ts.begin();
            s << q(BOLD + viua::libs::lexer::to_string(*it) + NORM);
            while (++it != ts.end()) {
                s << ", ";
                s << q(BOLD + viua::libs::lexer::to_string(*it) + NORM);
            }

            e.aside(s.str());
        }
        throw e;
    }
    auto lx = lexemes.front();
    lexemes.remove_prefix(1);
    return lx;
}

auto look_ahead [[maybe_unused]] (
    viua::libs::lexer::TOKEN const tk,
    viua::support::vector_view<viua::libs::lexer::Lexeme> const& lexemes)
-> bool
{
    return (not lexemes.empty()) and (lexemes.front() == tk);
}
auto look_ahead [[maybe_unused]] (
    std::set<viua::libs::lexer::TOKEN> const ts,
    viua::support::vector_view<viua::libs::lexer::Lexeme> const& lexemes)
-> bool
{
    return (not lexemes.empty()) and (ts.count(lexemes.front().token) != 0);
}

auto consume_one_attr(viua::support::vector_view<Lexeme>& lexemes)
    -> ast::Node::attribute_type
{
    using viua::libs::lexer::TOKEN;
    auto key   = consume_token_of(TOKEN::LITERAL_ATOM, lexemes);
    auto value = key.make_synth();
    if (look_ahead(TOKEN::EQ, lexemes)) {
        consume_token_of(TOKEN::EQ, lexemes);
        value = consume_token_of({TOKEN::LITERAL_INTEGER,
                                  TOKEN::LITERAL_STRING,
                                  TOKEN::LITERAL_ATOM},
                                 lexemes);
    }
    return {std::move(key), std::move(value)};
}
auto consume_attrs(
    viua::support::vector_view<viua::libs::lexer::Lexeme>& lexemes)
    -> std::vector<ast::Node::attribute_type>
{
    auto attrs = std::vector<ast::Node::attribute_type>{};

    /*
     * Empty attribute lists are not allowed.
     */
    using viua::libs::lexer::TOKEN;
    consume_token_of(TOKEN::ATTR_LIST_OPEN, lexemes);
    attrs.emplace_back(consume_one_attr(lexemes));
    while (not look_ahead(TOKEN::ATTR_LIST_CLOSE, lexemes)) {
        consume_token_of(TOKEN::COMMA, lexemes);
        attrs.emplace_back(consume_one_attr(lexemes));
    }
    consume_token_of(TOKEN::ATTR_LIST_CLOSE, lexemes);

    return attrs;
}

auto consume_switch_to_text(viua::support::vector_view<Lexeme>& lexemes)
    -> std::unique_ptr<ast::Section>
{
    using viua::libs::lexer::TOKEN;
    auto ss         = std::make_unique<ast::Section>();
    auto const base = consume_token_of(TOKEN::SWITCH_TO_TEXT, lexemes);
    ss->leader      = base.make_synth(".section", TOKEN::SWITCH_TO_SECTION);
    ss->name        = base.make_synth("\".text\"", TOKEN::LITERAL_STRING);
    consume_token_of(TOKEN::TERMINATOR, lexemes);
    return ss;
}
auto consume_switch_to_rodata(viua::support::vector_view<Lexeme>& lexemes)
    -> std::unique_ptr<ast::Section>
{
    using viua::libs::lexer::TOKEN;
    auto ss         = std::make_unique<ast::Section>();
    auto const base = consume_token_of(TOKEN::SWITCH_TO_RODATA, lexemes);
    ss->leader      = base.make_synth(".section", TOKEN::SWITCH_TO_SECTION);
    ss->name        = base.make_synth("\".rodata\"", TOKEN::LITERAL_STRING);
    consume_token_of(TOKEN::TERMINATOR, lexemes);
    return ss;
}
auto consume_switch_to_section(viua::support::vector_view<Lexeme>& lexemes)
    -> std::unique_ptr<ast::Section>
{
    using viua::libs::lexer::TOKEN;
    auto ss    = std::make_unique<ast::Section>();
    ss->leader = consume_token_of(TOKEN::SWITCH_TO_SECTION, lexemes);
    ss->name   = consume_token_of(TOKEN::LITERAL_STRING, lexemes);
    consume_token_of(TOKEN::TERMINATOR, lexemes);
    return ss;
}
auto consume_label(viua::support::vector_view<Lexeme>& lexemes)
    -> std::unique_ptr<ast::Label>
{
    using viua::libs::lexer::TOKEN;
    auto ss    = std::make_unique<ast::Label>();
    ss->leader = consume_token_of(TOKEN::DEFINE_LABEL, lexemes);
    ss->name =
        consume_token_of({TOKEN::LITERAL_ATOM, TOKEN::LITERAL_STRING}, lexemes);
    consume_token_of(TOKEN::TERMINATOR, lexemes);
    return ss;
}
auto consume_object(viua::support::vector_view<Lexeme>& lexemes)
    -> std::unique_ptr<ast::Object>
{
    using viua::libs::lexer::TOKEN;
    auto ss    = std::make_unique<ast::Object>();
    ss->leader = consume_token_of(TOKEN::ALLOCATE_OBJECT, lexemes);
    if (look_ahead(TOKEN::ATTR_LIST_OPEN, lexemes)) {
        ss->attributes = consume_attrs(lexemes);
    }
    ss->type = consume_token_of(TOKEN::LITERAL_ATOM, lexemes);
    while (not look_ahead(TOKEN::TERMINATOR, lexemes)) {
        ss->ctor.emplace_back(consume_token_of(TOKEN::LITERAL_STRING, lexemes));
    }
    consume_token_of(TOKEN::TERMINATOR, lexemes);
    return ss;
}
auto consume_symbol(viua::support::vector_view<Lexeme>& lexemes)
    -> std::unique_ptr<ast::Symbol>
{
    using viua::libs::lexer::TOKEN;
    auto ss    = std::make_unique<ast::Symbol>();
    ss->leader = consume_token_of(TOKEN::DECLARE_SYMBOL, lexemes);
    if (look_ahead(TOKEN::ATTR_LIST_OPEN, lexemes)) {
        ss->attributes = consume_attrs(lexemes);
    }
    ss->name =
        consume_token_of({TOKEN::LITERAL_ATOM, TOKEN::LITERAL_STRING}, lexemes);
    consume_token_of(TOKEN::TERMINATOR, lexemes);
    return ss;
}
auto consume_begin(viua::support::vector_view<Lexeme>& lexemes)
    -> std::unique_ptr<ast::Begin>
{
    using viua::libs::lexer::TOKEN;
    auto ss    = std::make_unique<ast::Begin>();
    ss->leader = consume_token_of(TOKEN::BEGIN, lexemes);
    consume_token_of(TOKEN::TERMINATOR, lexemes);
    return ss;
}
auto consume_end(viua::support::vector_view<Lexeme>& lexemes)
    -> std::unique_ptr<ast::End>
{
    using viua::libs::lexer::TOKEN;
    auto ss    = std::make_unique<ast::End>();
    ss->leader = consume_token_of(TOKEN::END, lexemes);
    consume_token_of(TOKEN::TERMINATOR, lexemes);
    return ss;
}
auto consume_instruction(viua::support::vector_view<Lexeme>& lexemes)
    -> std::unique_ptr<ast::Instruction>
{
    auto instruction = std::make_unique<ast::Instruction>();

    using viua::libs::lexer::TOKEN;
    try {
        instruction->leader = consume_token_of(TOKEN::OPCODE, lexemes);
    } catch (viua::libs::lexer::Lexeme const& e) {
        if (e.token != viua::libs::lexer::TOKEN::LITERAL_ATOM) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{e, Cause::Unexpected_token, e.text};
        }

        using viua::libs::lexer::OPCODE_NAMES;
        using viua::support::string::levenshtein_filter;
        auto misspell_candidates = levenshtein_filter(e.text, OPCODE_NAMES);
        if (misspell_candidates.empty()) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{e, Cause::Unexpected_token, e.text};
        }

        using viua::support::string::levenshtein_best;
        auto best_candidate =
            levenshtein_best(e.text, misspell_candidates, (e.text.size() / 2));
        if (best_candidate.second == e.text) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{e, Cause::Unexpected_token, e.text};
        }

        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw did_you_mean(Error{e, Cause::Unknown_opcode, e.text},
                           best_candidate.second);
    }

    /*
     * Special case for instructions with no operands. It is here to make
     * the loop that extracts the operands simpler.
     */
    if (lexemes.front() == TOKEN::TERMINATOR) {
        consume_token_of(TOKEN::TERMINATOR, lexemes);
        return instruction;
    }

    auto const fundamental_type_names = std::map<std::string, uint8_t>{
        {"int", static_cast<uint8_t>(viua::arch::FUNDAMENTAL_TYPES::INT)},
        {"uint", static_cast<uint8_t>(viua::arch::FUNDAMENTAL_TYPES::UINT)},
        {"float", static_cast<uint8_t>(viua::arch::FUNDAMENTAL_TYPES::FLOAT32)},
        {"double",
         static_cast<uint8_t>(viua::arch::FUNDAMENTAL_TYPES::FLOAT64)},
        {"pointer",
         static_cast<uint8_t>(viua::arch::FUNDAMENTAL_TYPES::POINTER)},
        {"atom", static_cast<uint8_t>(viua::arch::FUNDAMENTAL_TYPES::ATOM)},
        {"pid", static_cast<uint8_t>(viua::arch::FUNDAMENTAL_TYPES::PID)},
    };

    auto valid_cast =
        [&lexemes, &instruction, &fundamental_type_names]() -> bool {
        if (instruction->leader != "cast" and instruction->leader != "g.cast") {
            return false;
        }

        /*
         * The type specifier (which in some cases looks like an instruction
         * name) is only valid in the second position.
         */
        if (instruction->operands.size() != 1) {
            return false;
        }

        return fundamental_type_names.count(lexemes.front().text);
    };

    while ((not lexemes.empty()) and lexemes.front() != TOKEN::END) {
        if (lexemes.front().token == TOKEN::END) {
            break;
        }

        auto operand = ast::Operand{};

        /*
         * Attributes come before the element they describe, so let's try to
         * parse them before an operand.
         */
        if (lexemes.front() == TOKEN::ATTR_LIST_OPEN) {
            operand.attributes = consume_attrs(lexemes);
        }

        /*
         * Consume the operand: void, register access, a literal value. This
         * will supply some value for the instruction to work on. This chain
         * of if-else should handle valid operands - and ONLY operands, not
         * their separators.
         */
        if (lexemes.front() == TOKEN::REG_VOID) {
            operand.ingredients.push_back(
                consume_token_of(TOKEN::REG_VOID, lexemes));
        } else if (look_ahead(TOKEN::DOLLAR, lexemes)) {
            auto const leader = consume_token_of(TOKEN::DOLLAR, lexemes);
            auto index        = Lexeme{};
            try {
                index = consume_token_of(TOKEN::LITERAL_INTEGER, lexemes);
            } catch (viua::libs::lexer::Lexeme const& e) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{e, Cause::Invalid_register_access}
                    .add(leader)
                    .aside("register index must be an integer");
            }
            try {
                auto const n = std::stoul(index.text);
                if (n > viua::arch::MAX_REGISTER_INDEX) {
                    throw std::out_of_range{""};
                }
            } catch (std::out_of_range const&) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{index, Cause::Invalid_register_access}.aside(
                    "register index range is 0-"
                    + std::to_string(viua::arch::MAX_REGISTER_INDEX));
            }
            operand.ingredients.push_back(index);

            if (look_ahead(TOKEN::DOT, lexemes)) {
                operand.ingredients.push_back(
                    consume_token_of(TOKEN::DOT, lexemes));
                operand.ingredients.push_back(
                    consume_token_of(TOKEN::LITERAL_ATOM, lexemes));
            } else {
                /*
                 * We did not encounter a dot and the explicit specifier of
                 * register set, so let's consider the only other valid
                 * following tokens.
                 */
                if (not look_ahead({TOKEN::COMMA, TOKEN::TERMINATOR},
                                   lexemes)) {
                    /*
                     * Yeah, the token stream did not match expectations. Abort.
                     */
                    consume_token_of({TOKEN::COMMA, TOKEN::DOT}, lexemes);
                }
            }
        } else if (look_ahead(TOKEN::AT, lexemes)) {
            auto const access = consume_token_of(TOKEN::AT, lexemes);
            auto label        = viua::libs::lexer::Lexeme{};
            try {
                label = consume_token_of(
                    {TOKEN::LITERAL_ATOM, TOKEN::LITERAL_STRING}, lexemes);
            } catch (viua::libs::errors::compile_time::Error& e) {
                throw e.add(access).aside("label must be an atom or a string");
            }
            operand.ingredients.push_back(access);
            operand.ingredients.push_back(label);
        } else if (valid_cast()) {
            auto const value =
                consume_token_of({TOKEN::OPCODE, TOKEN::LITERAL_ATOM}, lexemes);
            operand.ingredients.push_back(value);
            auto& tt = operand.ingredients.front().text;
            tt       = std::to_string(fundamental_type_names.at(tt));
        } else if (lexemes.front() == TOKEN::LITERAL_INTEGER) {
            auto const value =
                consume_token_of(TOKEN::LITERAL_INTEGER, lexemes);
            operand.ingredients.push_back(value);
        } else if (lexemes.front() == TOKEN::LITERAL_FLOAT) {
            auto const value = consume_token_of(TOKEN::LITERAL_FLOAT, lexemes);
            operand.ingredients.push_back(value);
        } else if (lexemes.front() == TOKEN::LITERAL_STRING) {
            auto const value = consume_token_of(TOKEN::LITERAL_STRING, lexemes);
            operand.ingredients.push_back(value);
        } else if (lexemes.front() == TOKEN::LITERAL_ATOM) {
            auto const value = consume_token_of(TOKEN::LITERAL_ATOM, lexemes);
            operand.ingredients.push_back(value);
        } else {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{lexemes.front(), Cause::Unexpected_token};
        }

        instruction->operands.push_back(std::move(operand));

        /*
         * Consume either a comma (meaning that there will be some more
         * operands), or a terminator (meaning that there will be no more
         * operands).
         */
        if (lexemes.front() == TOKEN::COMMA) {
            auto const comma = consume_token_of(TOKEN::COMMA, lexemes);

            if (look_ahead(TOKEN::TERMINATOR, lexemes)) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{comma,
                            Cause::None,
                            "expected an operand to follow a comma"};
            }

            continue;
        }
        if (lexemes.front() == TOKEN::TERMINATOR) {
            consume_token_of(TOKEN::TERMINATOR, lexemes);
            break;
        }

        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{lexemes.front(), Cause::Unexpected_token}.note(
            "expected a comma, or a newline");
    }

    return instruction;
}
auto parse(viua::support::vector_view<Lexeme> lexemes)
    -> std::vector<std::unique_ptr<ast::Node>>
{
    auto nodes = std::vector<std::unique_ptr<ast::Node>>{};

    while (not lexemes.empty()) {
        auto const& each = lexemes.front();

        using viua::libs::lexer::TOKEN;
        switch (each.token) {
        case TOKEN::SWITCH_TO_TEXT:
            nodes.emplace_back(consume_switch_to_text(lexemes));
            break;
        case TOKEN::SWITCH_TO_RODATA:
            nodes.emplace_back(consume_switch_to_rodata(lexemes));
            break;
        case TOKEN::SWITCH_TO_SECTION:
            nodes.emplace_back(consume_switch_to_section(lexemes));
            break;
        case TOKEN::DEFINE_LABEL:
            nodes.emplace_back(consume_label(lexemes));
            break;
        case TOKEN::ALLOCATE_OBJECT:
            nodes.emplace_back(consume_object(lexemes));
            break;
        case TOKEN::DECLARE_SYMBOL:
            nodes.emplace_back(consume_symbol(lexemes));
            break;
        case TOKEN::BEGIN:
            nodes.emplace_back(consume_begin(lexemes));
            break;
        case TOKEN::END:
            nodes.emplace_back(consume_end(lexemes));
            break;
        case TOKEN::OPCODE:
            nodes.emplace_back(consume_instruction(lexemes));
            break;
        default:
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{each, Cause::Unexpected_token}.note(
                "refer to viua-asm-lang(1) for more information");
        }
    }

    return nodes;
}

auto make_symbol(std::string_view const name,
                 std::vector<uint8_t>& string_table) -> Elf64_Sym
{
    auto const name_off =
        name.empty() ? 0 : save_string_to_strtab(string_table, name);

    auto sym    = Elf64_Sym{};
    sym.st_name = name_off;
    return sym;
}

auto save_declared_symbols(std::vector<std::unique_ptr<ast::Node>> const& nodes,
                           std::vector<uint8_t>& string_table,
                           std::vector<Elf64_Sym>& symbol_table,
                           std::map<std::string, size_t>& symbol_map,
                           std::vector<ast::Symbol>& declarations) -> void
{
    enum class SECTION {
        NONE,
        RODATA,
        TEXT,
    };
    auto active_section = SECTION::NONE;
    auto activator      = std::experimental::observer_ptr<ast::Section const>{};
    for (auto const& each : nodes) {
        using viua::libs::lexer::TOKEN;

        if (each->leader.token == TOKEN::SWITCH_TO_SECTION) {
            activator.reset(static_cast<ast::Section const*>(each.get()));
            auto const& sec = *activator;
            auto const name = sec.which();
            if (name == std::string_view{".rodata"}) {
                active_section = SECTION::RODATA;
            } else if (name == std::string_view{".text"}) {
                active_section = SECTION::TEXT;
            } else {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{sec.name,
                            Cause::None,
                            "unknown section: " + quote_fancy(name)};
            }
            continue;
        }

        if (each->leader.token != TOKEN::DECLARE_SYMBOL) {
            continue;
        }
        if (active_section == SECTION::NONE) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{each->leader, Cause::None, "no active section"};
        }

        auto const& sym = static_cast<ast::Symbol&>(*each);

        auto const name_off =
            save_string_to_strtab(string_table, sym.name.text);

        auto type = (active_section == SECTION::TEXT) ? STT_FUNC : STT_OBJECT;
        auto binding    = (active_section == SECTION::TEXT) ? STB_GLOBAL
                                                            : STB_LOCAL;
        auto visibility = STV_DEFAULT;

        if (sym.has_attr("local") and sym.has_attr("global")) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            auto const glo = sym.has_attr("global").value();
            auto const loc = sym.has_attr("local").value();
            auto const [earlier, later] =
                (glo.first.location.offset < loc.first.location.offset)
                    ? std::pair{glo, loc}
                    : std::pair{loc, glo};

            auto e = Error{later.first,
                           Cause::None,
                           "symbols cannot be explicitly local and global"};
            e.add(earlier.first);
            e.aside("remove one of the tags");
            throw e;
        } else if (sym.has_attr("local")) {
            binding = STB_LOCAL;
        } else if (sym.has_attr("global")) {
            binding = STB_GLOBAL;
        }

        if (sym.has_attr("hidden")) {
            visibility = STV_HIDDEN;
        }

        if (binding == STB_GLOBAL and type == STT_OBJECT
            and visibility != STV_HIDDEN) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            auto e = Error{sym.has_attr("global")
                               .value_or(std::pair{each->leader, std::nullopt})
                               .first,
                           Cause::None,
                           "object symbols cannot be globally visible"};
            e.chain(std::move(Error{activator->leader, Cause::None}.note(
                "section activated here")));

            if (auto const g = sym.has_attr("global"); g) {
                e.add(each->leader);
                e.aside("remove tag or change it to explicit "
                        + quote_fancy("local"));
            }

            throw e;
        }

        auto symbol     = Elf64_Sym{};
        symbol.st_name  = name_off;
        symbol.st_info  = ELF64_ST_INFO(binding, type);
        symbol.st_other = visibility;

        /*
         * Leave size and offset of the symbol empty since we do not have this
         * information yet. It will only be available after the whole ELF is
         * emitted.
         *
         * For symbols marked as [[extern]] st_value will be LEFT EMPTY after
         * the assembler exits, as a signal to the linker that this symbol was
         * defined in a different module and needs to be resolved.
         */
        symbol.st_size  = 0;
        symbol.st_value = 0;

        /*
         * Section header table index (see elf(5) for st_shndx) is filled
         * out later, since at this point we do not have this information.
         *
         * For variables it will be the index of .rodata.
         * For functions it will be the index of .text.
         */
        symbol.st_shndx = 0;

        viua::libs::stage::record_symbol(
            sym.name.text, symbol, symbol_table, symbol_map);
        declarations.emplace_back(sym);
    }
}

auto save_objects(std::vector<std::unique_ptr<ast::Node>> const& nodes,
                  std::vector<uint8_t>& rodata_buf,
                  std::vector<uint8_t>& string_table,
                  std::vector<Elf64_Sym>& symbol_table,
                  std::map<std::string, size_t>& symbol_map) -> void
{
    enum class SECTION {
        NONE,
        RODATA,
        TEXT,
    };
    auto active_section = SECTION::NONE;
    auto activator      = std::experimental::observer_ptr<ast::Section const>{};

    auto active_label = std::string{};
    auto labeller     = std::experimental::observer_ptr<ast::Label const>{};

    auto active_symbol = std::experimental::observer_ptr<Elf64_Sym>{};

    for (auto const& each : nodes) {
        using viua::libs::lexer::TOKEN;

        if (each->leader.token == TOKEN::SWITCH_TO_SECTION) {
            activator.reset(static_cast<ast::Section const*>(each.get()));
            auto const& sec = *activator;
            auto const name = sec.which();
            if (name == std::string_view{".rodata"}) {
                active_section = SECTION::RODATA;
            } else if (name == std::string_view{".text"}) {
                active_section = SECTION::TEXT;
            } else {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{sec.name,
                            Cause::None,
                            "unknown section: " + quote_fancy(name)};
            }
            continue;
        }

        auto const is_label = (each->leader.token == TOKEN::DEFINE_LABEL);
        auto const is_alloc = (each->leader.token == TOKEN::ALLOCATE_OBJECT);
        if (auto const is_interesting = is_label or is_alloc;
            not is_interesting) {
            continue;
        }

        if (active_section == SECTION::NONE) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{each->leader, Cause::None, "no active section"};
        }
        if (active_section == SECTION::TEXT) {
            continue;
        }

        if (is_label) {
            auto const& lab = static_cast<ast::Label&>(*each);
            labeller.reset(&lab);
            active_label = lab.name.text;

            if (not symbol_map.contains(active_label)) {
                auto local_sym     = make_symbol(active_label, string_table);
                local_sym.st_info  = ELF64_ST_INFO(STB_LOCAL, STT_OBJECT);
                local_sym.st_other = STV_DEFAULT;
                auto sym_ndx       = viua::libs::stage::record_symbol(
                    active_label, local_sym, symbol_table, symbol_map);
                active_symbol.reset(&symbol_table.at(sym_ndx));
            } else {
                active_symbol.reset(
                    &symbol_table.at(symbol_map.at(active_label)));
            }

            std::cerr << "recording object label " << active_label << " at "
                      << "[.rodata+0x" << std::hex << std::setfill('0')
                      << std::setw(16) << rodata_buf.size() << std::dec
                      << std::setfill(' ') << "]\n";

            active_symbol->st_value = rodata_buf.size();
            continue;
        }

        if (is_alloc) {
            auto const& alc = static_cast<ast::Object&>(*each);
            std::cerr << "allocating " << alc.type.text << " under label "
                      << active_label << "\n";

            if (alc.type.text == "string") {
                auto s = std::string{};
                for (auto i = size_t{0}; i < alc.ctor.size(); ++i) {
                    auto& each = alc.ctor.at(i);

                    using enum viua::libs::lexer::TOKEN;
                    if (each.token == LITERAL_STRING) {
                        auto tmp = each.text;
                        tmp      = tmp.substr(1, tmp.size() - 2);
                        tmp      = viua::support::string::unescape(tmp);
                        s += tmp;
                    } else if (each.token == STAR) {
                        auto& next = alc.ctor.at(++i);
                        if (next.token != LITERAL_INTEGER) {
                            using viua::libs::errors::compile_time::Cause;
                            using viua::libs::errors::compile_time::Error;

                            throw Error{each,
                                        Cause::Invalid_operand,
                                        "cannot multiply string constant "
                                        "by non-integer"}
                                .add(next)
                                .add(alc.ctor.at(i - 2))
                                .aside("right-hand side must be an "
                                       "positive integer");
                        }

                        auto x = viua::support::ston<size_t>(next.text);
                        auto o = std::ostringstream{};
                        for (auto i = size_t{0}; i < x; ++i) {
                            o << s;
                        }
                        s += o.str();
                    }
                }

                auto const value_off = save_buffer_to_rodata(rodata_buf, s);
                if (active_symbol) {
                    active_symbol->st_value = value_off;
                    active_symbol->st_size =
                        (rodata_buf.size() - active_symbol->st_value);
                }

                std::cerr << "allocated " << active_symbol->st_size
                          << " byte(s) of " << alc.type.text << " under label "
                          << active_label << " at "
                          << "[.rodata+0x" << std::hex << std::setfill('0')
                          << std::setw(16) << active_symbol->st_value
                          << std::dec << std::setfill(' ') << "]\n";
                active_symbol.reset();
            }
        }
    }
}

auto cache_function_labels(std::vector<std::unique_ptr<ast::Node>> const& nodes,
                           std::vector<uint8_t>& string_table,
                           std::vector<Elf64_Sym>& symbol_table,
                           std::map<std::string, size_t>& symbol_map) -> void
{
    enum class SECTION {
        NONE,
        RODATA,
        TEXT,
    };
    auto active_section = SECTION::NONE;
    auto activator      = std::experimental::observer_ptr<ast::Section const>{};

    auto active_label = std::string{};
    auto labeller     = std::experimental::observer_ptr<ast::Label const>{};

    auto active_symbol = std::experimental::observer_ptr<Elf64_Sym>{};

    for (auto const& each : nodes) {
        using viua::libs::lexer::TOKEN;

        if (each->leader.token == TOKEN::SWITCH_TO_SECTION) {
            activator.reset(static_cast<ast::Section const*>(each.get()));
            auto const& sec = *activator;
            auto const name = sec.which();
            if (name == std::string_view{".rodata"}) {
                active_section = SECTION::RODATA;
            } else if (name == std::string_view{".text"}) {
                active_section = SECTION::TEXT;
            } else {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{sec.name,
                            Cause::None,
                            "unknown section: " + quote_fancy(name)};
            }
            continue;
        }

        auto const is_label = (each->leader.token == TOKEN::DEFINE_LABEL);
        if (auto const is_interesting = is_label; not is_interesting) {
            continue;
        }

        if (active_section == SECTION::NONE) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{each->leader, Cause::None, "no active section"};
        }
        if (active_section == SECTION::RODATA) {
            continue;
        }

        if (is_label) {
            auto const& lab = static_cast<ast::Label&>(*each);
            labeller.reset(&lab);
            active_label = lab.name.text;

            if (not symbol_map.contains(active_label)) {
                auto local_sym     = make_symbol(active_label, string_table);
                local_sym.st_info  = ELF64_ST_INFO(STB_LOCAL, STT_FUNC);
                local_sym.st_other = STV_HIDDEN;
                auto sym_ndx       = viua::libs::stage::record_symbol(
                    active_label, local_sym, symbol_table, symbol_map);
                active_symbol.reset(&symbol_table.at(sym_ndx));
            } else {
                active_symbol.reset(
                    &symbol_table.at(symbol_map.at(active_label)));
            }

            std::cerr << "caching "
                      << ((active_symbol->st_other == STV_DEFAULT) ? "function"
                                                                   : "jump")
                      << " label " << active_label << "\n";
            continue;
        }
    }
}

using Text = std::vector<viua::arch::instruction_type>;
auto cook_instructions(std::vector<std::unique_ptr<ast::Node>> const& nodes,
                       std::vector<uint8_t>& rodata_buf,
                       std::vector<uint8_t>& string_table,
                       std::vector<Elf64_Sym>& symbol_table,
                       std::map<std::string, size_t>& symbol_map) -> Text
{
    enum class SECTION {
        NONE,
        RODATA,
        TEXT,
    };
    auto active_section = SECTION::NONE;
    auto activator      = std::experimental::observer_ptr<ast::Section const>{};

    auto active_function = std::experimental::observer_ptr<Elf64_Sym>{};
    auto function_label  = std::experimental::observer_ptr<ast::Label const>{};

    auto active_symbol = std::experimental::observer_ptr<Elf64_Sym>{};
    auto active_label  = std::string{};
    auto labeller      = std::experimental::observer_ptr<ast::Label const>{};

    auto text = Text{};
    {
        using viua::arch::instruction_type;
        using viua::arch::ops::N;
        using viua::arch::ops::OPCODE;
        text.emplace_back(
            N{static_cast<instruction_type>(OPCODE::HALT)}.encode());
    }

    (void)rodata_buf;
    (void)string_table;

    auto const save_size_of_active_function =
        [&text, &af = active_function, &function_label]() -> void {
        if (af) {
            af->st_size = ((text.size() * sizeof(viua::arch::instruction_type))
                           - af->st_value);

            std::cerr << "  size of function " << function_label->name.text
                      << " is " << af->st_size << " bytes\n";
            std::cerr << "  span of function " << function_label->name.text
                      << " is "
                      << "[.text+0x" << std::hex << std::setfill('0')
                      << std::setw(16) << af->st_value << "..." << std::setw(16)
                      << (af->st_value + af->st_size
                          - sizeof(viua::arch::instruction_type))
                      << std::dec << std::setfill(' ') << "]\n";
        }
    };

    for (auto const& each : nodes) {
        using viua::libs::lexer::TOKEN;

        if (each->leader.token == TOKEN::SWITCH_TO_SECTION) {
            activator.reset(static_cast<ast::Section const*>(each.get()));
            auto const& sec = *activator;
            auto const name = sec.which();
            if (name == std::string_view{".rodata"}) {
                active_section = SECTION::RODATA;
            } else if (name == std::string_view{".text"}) {
                active_section = SECTION::TEXT;
            } else {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{sec.name,
                            Cause::None,
                            "unknown section: " + quote_fancy(name)};
            }
            continue;
        }

        auto const is_label       = (each->leader.token == TOKEN::DEFINE_LABEL);
        auto const is_instruction = (each->leader.token == TOKEN::OPCODE);
        if (auto const is_interesting = is_label or is_instruction;
            not is_interesting) {
            continue;
        }

        if (active_section == SECTION::NONE) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{each->leader, Cause::None, "no active section"};
        }
        if (active_section == SECTION::RODATA) {
            continue;
        }

        if (is_label) {
            auto const& lab = static_cast<ast::Label&>(*each);
            labeller.reset(&lab);
            active_label = lab.name.text;

            auto labelled_symbol = std::experimental::observer_ptr<Elf64_Sym>{};
            if (symbol_map.contains(active_label)) {
                labelled_symbol.reset(
                    &symbol_table.at(symbol_map.at(active_label)));
            } else {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{lab.name,
                            Cause::None,
                            "text label without attached symbol"};
            }

            labelled_symbol->st_value =
                (text.size() * sizeof(viua::arch::instruction_type));

            auto const is_jump_label =
                (ELF64_ST_BIND(labelled_symbol->st_info) == STB_LOCAL)
                and (labelled_symbol->st_other == STV_HIDDEN);
            auto const is_function_label = not is_jump_label;
            if (is_function_label) {
                save_size_of_active_function();
                active_function = labelled_symbol;
                function_label.reset(&lab);
            }

            std::cerr << (is_jump_label ? "  " : "") << "recording "
                      << (is_jump_label ? "jump" : "call") << " label "
                      << active_label << " at "
                      << "[.text+0x" << std::hex << std::setfill('0')
                      << std::setw(16)
                      << (text.size() * sizeof(viua::arch::instruction_type))
                      << std::dec << std::setfill(' ') << "]\n";

            active_symbol = labelled_symbol;

            continue;
        }

        if (is_instruction) {
            auto const& instr = static_cast<ast::Instruction&>(*each);
            std::cerr << "    cooking " << instr.leader.text << "\n";

            text.push_back(0);
        }
    }
    save_size_of_active_function();

    {
        using viua::arch::instruction_type;
        using viua::arch::ops::N;
        using viua::arch::ops::OPCODE;
        text.emplace_back(
            N{static_cast<instruction_type>(OPCODE::HALT)}.encode());
    }

    return text;
}

auto find_entry_point(std::vector<std::unique_ptr<ast::Node>> const& nodes,
                      std::vector<Elf64_Sym>& symbol_table,
                      std::map<std::string, size_t>& symbol_map)
    -> std::optional<viua::libs::lexer::Lexeme>
{
    auto entry_point_fn = std::optional<viua::libs::lexer::Lexeme>{};
    for (auto const& each : nodes) {
        using viua::libs::lexer::TOKEN;
        auto const is_symbol = (each->leader.token == TOKEN::DECLARE_SYMBOL);
        auto const is_entry_point = each->has_attr("entry_point");

        if (is_entry_point and not is_symbol) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            throw Error{each->leader,
                        Cause::None,
                        "only .symbol can be tagged as entry point"}
                .add(*each->attr("entry_point"));
        }

        if (is_entry_point) {
            if (entry_point_fn.has_value()) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;

                auto const dup = static_cast<ast::Symbol const&>(*each);
                throw Error{
                    dup.name, Cause::Duplicated_entry_point, dup.name.text}
                    .add(dup.attr("entry_point").value())
                    .note("first entry point was: " + entry_point_fn->text);
            }
            entry_point_fn = static_cast<ast::Symbol const&>(*each).name;

            auto const sym =
                symbol_table.at(symbol_map.at(entry_point_fn->text));
            auto const not_global  = (ELF64_ST_BIND(sym.st_info) != STB_GLOBAL);
            auto const not_visible = (sym.st_other != STV_DEFAULT);
            auto const not_function = (ELF64_ST_TYPE(sym.st_info) != STT_FUNC);
            if (not_function) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;

                throw Error{*entry_point_fn,
                            Cause::None,
                            "entry point must be a function"}
                    .add(*each->attr("entry_point"));
            }
            if (not_global) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;

                auto e = Error{*each->attr("local"),
                               Cause::None,
                               "entry point must be global"};
                e.add(*each->attr("entry_point"));
                e.add(*entry_point_fn, true);
                e.aside("remove the " + quote_fancy("local")
                        + " tag, or change it to explicit "
                        + quote_fancy("global"));
                throw e;
            }
            if (not_visible) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;

                auto e = Error{*each->attr("hidden"),
                               Cause::None,
                               "entry point must not be hidden"};
                e.add(*each->attr("entry_point"));
                e.add(*entry_point_fn, true);
                e.aside("remove the " + quote_fancy("hidden") + " tag");
                throw e;
            }
        }
    }
    return entry_point_fn;
}

auto make_reloc_table(Text const& text) -> std::vector<Elf64_Rel>
{
    auto reloc_table = std::vector<Elf64_Rel>{};

    auto const push_reloc = [&text, &reloc_table](size_t const i) -> void {
        using viua::arch::ops::F;
        auto const hi =
            static_cast<uint64_t>(F::decode(text.at(i - 2)).immediate) << 32;
        auto const lo                 = F::decode(text.at(i - 1)).immediate;
        auto const symtab_entry_index = static_cast<uint32_t>(hi | lo);

        using viua::arch::ops::OPCODE;
        auto const op =
            static_cast<OPCODE>(text.at(i) & viua::arch::ops::OPCODE_MASK);

        using enum viua::arch::elf::R_VIUA;
        auto const type = (op == OPCODE::ATOM) ? R_VIUA_OBJECT
                                               : R_VIUA_JUMP_SLOT;

        Elf64_Rel rel;
        rel.r_offset = (i - 2) * sizeof(viua::arch::instruction_type);
        rel.r_info =
            ELF64_R_INFO(symtab_entry_index, static_cast<uint8_t>(type));
        reloc_table.push_back(rel);
    };

    for (auto i = size_t{0}; i < text.size(); ++i) {
        using viua::arch::opcode_type;
        using viua::arch::ops::OPCODE;

        auto const each = text.at(i);

        auto const op =
            static_cast<OPCODE>(each & viua::arch::ops::OPCODE_MASK);
        switch (op) {
            using enum viua::arch::ops::OPCODE;
        case CALL:
        case ATOM:
            push_reloc(i);
            break;
        default:
            break;
        }
    }

    return reloc_table;
}

auto emit_elf(std::filesystem::path const output_path,
              bool const as_executable,
              std::optional<uint64_t> const entry_point_fn,
              Text const& text,
              std::optional<std::vector<Elf64_Rel>> relocs,
              std::vector<uint8_t> const& rodata_buf,
              std::vector<uint8_t> const& string_table,
              std::vector<Elf64_Sym>& symbol_table) -> void
{
    auto const a_out = open(output_path.c_str(),
                            O_CREAT | O_TRUNC | O_WRONLY,
                            S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (a_out == -1) {
        close(a_out);
        exit(1);
    }

    constexpr auto VIUA_MAGIC [[maybe_unused]] = "\x7fVIUA\x00\x00\x00";
    auto const VIUAVM_INTERP                   = std::string{"viua-vm"};
    auto const VIUA_COMMENT = std::string{VIUAVM_VERSION_FULL};

    {
        // see elf(5)
        Elf64_Ehdr elf_header{};
        elf_header.e_ident[EI_MAG0]       = '\x7f';
        elf_header.e_ident[EI_MAG1]       = 'E';
        elf_header.e_ident[EI_MAG2]       = 'L';
        elf_header.e_ident[EI_MAG3]       = 'F';
        elf_header.e_ident[EI_CLASS]      = ELFCLASS64;
        elf_header.e_ident[EI_DATA]       = ELFDATA2LSB;
        elf_header.e_ident[EI_VERSION]    = EV_CURRENT;
        elf_header.e_ident[EI_OSABI]      = ELFOSABI_STANDALONE;
        elf_header.e_ident[EI_ABIVERSION] = 0;
        elf_header.e_type                 = (as_executable ? ET_EXEC : ET_REL);
        elf_header.e_machine              = ET_NONE;
        elf_header.e_version              = elf_header.e_ident[EI_VERSION];
        elf_header.e_flags  = 0;  // processor-specific flags, should be 0
        elf_header.e_ehsize = sizeof(elf_header);

        auto shstr            = std::vector<char>{'\0'};
        auto save_shstr_entry = [&shstr](std::string_view const sv) -> size_t {
            auto const saved_at = shstr.size();
            std::copy(sv.begin(), sv.end(), std::back_inserter(shstr));
            shstr.push_back('\0');
            return saved_at;
        };

        auto text_section_ndx   = size_t{0};
        auto rel_section_ndx    = size_t{0};
        auto rodata_section_ndx = size_t{0};
        auto symtab_section_ndx = size_t{0};
        auto strtab_section_ndx = size_t{0};

        using Header_pair = std::pair<std::optional<Elf64_Phdr>, Elf64_Shdr>;
        auto elf_headers  = std::vector<Header_pair>{};

        {
            /*
             * It is mandated by ELF that the first section header is void, and
             * must be all zeroes. It is reserved and used by ELF extensions.
             *
             * We do not extend ELF in any way, so this section is SHT_NULL for
             * Viua VM.
             */
            Elf64_Phdr seg{};
            seg.p_type   = PT_NULL;
            seg.p_offset = 0;
            seg.p_filesz = 0;

            Elf64_Shdr void_section{};
            void_section.sh_type = SHT_NULL;

            elf_headers.push_back({seg, void_section});
        }
        {
            /*
             * .viua.magic
             *
             * The second section (and the first fragment) is the magic number
             * Viua uses to detect the if the binary *really* is something it
             * can handle, and on Linux by the binfmt.d(5) to enable running
             * Viua ELFs automatically.
             */
            Elf64_Phdr seg{};
            seg.p_type   = PT_NULL;
            seg.p_offset = 0;
            memcpy(&seg.p_offset, VIUA_MAGIC, 8);
            seg.p_filesz = 8;

            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".viua.magic");
            sec.sh_type   = SHT_NOBITS;
            sec.sh_offset = sizeof(Elf64_Ehdr) + offsetof(Elf64_Phdr, p_offset);
            sec.sh_size   = 8;
            sec.sh_flags  = 0;

            elf_headers.push_back({seg, sec});
        }
        {
            /*
             * .interp
             *
             * What follows is the interpreter. This is mostly useful to get
             * better reporting out of readelf(1) and file(1). It also serves as
             * a second thing to check for if the file *really* is a Viua
             * binary.
             */

            Elf64_Phdr seg{};
            seg.p_type   = PT_INTERP;
            seg.p_offset = 0;
            seg.p_filesz = VIUAVM_INTERP.size() + 1;
            seg.p_flags  = PF_R;

            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".interp");
            sec.sh_type   = SHT_PROGBITS;
            sec.sh_offset = 0;
            sec.sh_size   = VIUAVM_INTERP.size() + 1;
            sec.sh_flags  = 0;

            elf_headers.push_back({seg, sec});
        }
        if (relocs.has_value()) {
            /*
             * .rel
             */
            auto const relocation_table = *relocs;

            Elf64_Shdr sec{};
            sec.sh_name    = save_shstr_entry(".rel");
            sec.sh_type    = SHT_REL;
            sec.sh_offset  = 0;
            sec.sh_entsize = sizeof(decltype(relocation_table)::value_type);
            sec.sh_size    = (relocation_table.size() * sec.sh_entsize);
            sec.sh_flags   = SHF_INFO_LINK;

            /*
             * This should point to .symtab section that is relevant for the
             * relocations contained in this .rel section (in our case its the
             * only .symtab section in the ELF), but we do not know that
             * section's index yet.
             */
            sec.sh_link = 0;

            /*
             * This should point to .text section (or any other section) to
             * which the relocations apply. We do not know that index yet, but
             * it MUST be patched later.
             */
            sec.sh_info = 0;

            rel_section_ndx = elf_headers.size();
            elf_headers.push_back({std::nullopt, sec});
        }
        {
            /*
             * .text
             *
             * The first segment and section pair that contains something users
             * of Viua can affect is the .text section ie, the executable
             * instructions representing user programs.
             */
            Elf64_Phdr seg{};
            seg.p_type   = PT_LOAD;
            seg.p_offset = 0;
            auto const sz =
                (text.size()
                 * sizeof(std::decay_t<decltype(text)>::value_type));
            seg.p_filesz = seg.p_memsz = sz;
            seg.p_flags                = PF_R | PF_X;
            seg.p_align                = sizeof(viua::arch::instruction_type);

            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".text");
            sec.sh_type   = SHT_PROGBITS;
            sec.sh_offset = 0;
            sec.sh_size   = seg.p_filesz;
            sec.sh_flags  = SHF_ALLOC | SHF_EXECINSTR;

            text_section_ndx = elf_headers.size();
            elf_headers.push_back({seg, sec});
        }
        {
            /*
             * .rodata
             *
             * Then, the .rodata section containing user data. Only constants
             * are allowed to be defined as data labels in Viua -- there are no
             * global variables.
             *
             * The "strings table" contains not only strings but also floats,
             * atoms, and any other piece of data that does not fit into a
             * single load instruction (with the exception of long integers
             * which are loaded using a sequence of raw instructions - this
             * allows loading addresses, which are then used to index strings
             * table).
             */
            Elf64_Phdr seg{};
            seg.p_type    = PT_LOAD;
            seg.p_offset  = 0;
            auto const sz = rodata_buf.size();
            seg.p_filesz = seg.p_memsz = sz;
            seg.p_flags                = PF_R;
            seg.p_align                = sizeof(viua::arch::instruction_type);

            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".rodata");
            sec.sh_type   = SHT_PROGBITS;
            sec.sh_offset = 0;
            sec.sh_size   = seg.p_filesz;
            sec.sh_flags  = SHF_ALLOC;

            rodata_section_ndx = elf_headers.size();
            elf_headers.push_back({seg, sec});
        }
        {
            /*
             * .comment
             */
            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".comment");
            sec.sh_type   = SHT_PROGBITS;
            sec.sh_offset = 0;
            sec.sh_size   = VIUA_COMMENT.size() + 1;
            sec.sh_flags  = 0;

            elf_headers.push_back({std::nullopt, sec});
        }
        {
            /*
             * .symtab
             *
             * Last, but not least, comes another LOAD segment.
             * It contains a symbol table with function addresses.
             *
             * Function calls use this table to determine the address to which
             * they should transfer control - there are no direct calls.
             * Inefficient, but flexible.
             */
            Elf64_Shdr sec{};
            sec.sh_name = save_shstr_entry(".symtab");
            /*
             * This could be SHT_SYMTAB, but the SHT_SYMTAB type sections expect
             * a certain format of the symbol table which Viua does not use. So
             * let's just use SHT_PROGBITS because interpretation of
             * SHT_PROGBITS is up to the program.
             */
            sec.sh_type    = SHT_SYMTAB;
            sec.sh_offset  = 0;
            sec.sh_size    = (symbol_table.size() * sizeof(Elf64_Sym));
            sec.sh_flags   = 0;
            sec.sh_entsize = sizeof(Elf64_Sym);
            sec.sh_info    = 0;

            symtab_section_ndx = elf_headers.size();
            elf_headers.push_back({std::nullopt, sec});
        }
        {
            /*
             * .strtab
             */
            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".strtab");
            sec.sh_type   = SHT_STRTAB;
            sec.sh_offset = 0;
            sec.sh_size   = string_table.size();
            sec.sh_flags  = SHF_STRINGS;

            strtab_section_ndx = elf_headers.size();
            elf_headers.push_back({std::nullopt, sec});
        }
        {
            /*
             * .shstrtab
             *
             * ACHTUNG! ATTENTION! UWAGA! POZOR! TÄHELEPANU!
             *
             * This section contains the strings table representing section
             * names. If any more sections are added they MUST APPEAR BEFORE
             * THIS SECTION. Otherwise the strings won't be available because
             * the size of the section will not be correct and will appear as
             * <corrupt> in readelf(1) output.
             */
            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".shstrtab");
            sec.sh_type   = SHT_STRTAB;
            sec.sh_offset = 0;
            sec.sh_size   = shstr.size();
            sec.sh_flags  = SHF_STRINGS;

            elf_headers.push_back({std::nullopt, sec});
        }

        /*
         * Link the .symtab to its associated .strtab; otherwise you will
         * get <corrupt> names when invoking readelf(1) to inspect the file.
         */
        elf_headers.at(symtab_section_ndx).second.sh_link = strtab_section_ndx;

        /*
         * Patch the symbol table section index.
         */
        if (relocs.has_value()) {
            elf_headers.at(rel_section_ndx).second.sh_link = symtab_section_ndx;
            elf_headers.at(rel_section_ndx).second.sh_info = text_section_ndx;
        }

        auto elf_pheaders = std::count_if(
            elf_headers.begin(),
            elf_headers.end(),
            [](auto const& each) -> bool { return each.first.has_value(); });
        auto elf_sheaders = elf_headers.size();

        auto const elf_size = sizeof(Elf64_Ehdr)
                              + (elf_pheaders * sizeof(Elf64_Phdr))
                              + (elf_sheaders * sizeof(Elf64_Shdr));
        auto text_offset = std::optional<size_t>{};
        {
            auto offset_accumulator = size_t{0};
            for (auto& [segment, section] : elf_headers) {
                if (segment.has_value() and (segment->p_type != PT_NULL)) {
                    if (segment->p_type == PT_NULL) {
                        continue;
                    }

                    /*
                     * The thing that Viua VM mandates is that the main function
                     * (if it exists) MUST be put in the first executable
                     * segment. This can be elegantly achieved by blindly
                     * pushing the address of first such segment.
                     *
                     * The following construction using std::optional:
                     *
                     *      x = x.value_or(y)
                     *
                     * ensures that x will store the first assigned value
                     * without any checks. Why not use somethin more C-like? For
                     * example:
                     *
                     *      x = (x ? x : y)
                     *
                     * looks like it achieves the same without any fancy-shmancy
                     * types. Yeah, it only looks like it does so. If the first
                     * executable segment would happen to be at offset 0 then
                     * the C-style code fails, while the C++-style is correct.
                     * As an aside: this ie, C style being broken an C++ being
                     * correct is something surprisingly common. Or rather more
                     * functional style being correct... But I digress.
                     */
                    if (segment->p_flags == (PF_R | PF_X)) {
                        text_offset = text_offset.value_or(offset_accumulator);
                    }

                    segment->p_offset = (elf_size + offset_accumulator);
                }

                if (section.sh_type == SHT_NULL) {
                    continue;
                }
                if (section.sh_type == SHT_NOBITS) {
                    continue;
                }

                section.sh_offset = (elf_size + offset_accumulator);
                offset_accumulator += section.sh_size;
            }
        }

        elf_header.e_entry = entry_point_fn.has_value()
                                 ? (*text_offset + *entry_point_fn + elf_size)
                                 : 0;

        elf_header.e_phoff     = sizeof(Elf64_Ehdr);
        elf_header.e_phentsize = sizeof(Elf64_Phdr);
        elf_header.e_phnum     = elf_pheaders;

        elf_header.e_shoff =
            elf_header.e_phoff + (elf_pheaders * sizeof(Elf64_Phdr));
        elf_header.e_shentsize = sizeof(Elf64_Shdr);
        elf_header.e_shnum     = elf_sheaders;
        elf_header.e_shstrndx  = elf_sheaders - 1;

        write(a_out, &elf_header, sizeof(elf_header));

        /*
         * Unfortunately, we have to have use two loops here because segment and
         * section headers cannot be interweaved. We could do some lseek(2)
         * tricks, but I don't think it's worth it. For-each loops are simple
         * and do not require any special bookkeeping to work correctly.
         */
        for (auto const& [segment, _] : elf_headers) {
            if (not segment) {
                continue;
            }
            write(a_out,
                  &*segment,
                  sizeof(std::remove_reference_t<decltype(*segment)>));
        }
        for (auto const& [_, section] : elf_headers) {
            write(a_out,
                  &section,
                  sizeof(std::remove_reference_t<decltype(section)>));
        }

        write(a_out, VIUAVM_INTERP.c_str(), VIUAVM_INTERP.size() + 1);

        if (relocs.has_value()) {
            for (auto const& rel : *relocs) {
                write(a_out, &rel, sizeof(std::decay_t<decltype(rel)>));
            }
        }

        auto const text_size =
            (text.size() * sizeof(std::decay_t<decltype(text)>::value_type));
        write(a_out, text.data(), text_size);

        write(a_out, rodata_buf.data(), rodata_buf.size());

        write(a_out, VIUA_COMMENT.c_str(), VIUA_COMMENT.size() + 1);

        for (auto& each : symbol_table) {
            switch (ELF64_ST_TYPE(each.st_info)) {
            case STT_FUNC:
                each.st_shndx = text_section_ndx;
                break;
            case STT_OBJECT:
                each.st_shndx = rodata_section_ndx;
                break;
            default:
                break;
            }
            write(a_out, &each, sizeof(std::decay_t<decltype(symbol_table)>));
        }

        write(a_out, string_table.data(), string_table.size());

        write(a_out, shstr.data(), shstr.size());
    }

    close(a_out);
}
}  // namespace

auto main(int argc, char* argv[]) -> int
{
    using viua::support::tty::ATTR_RESET;
    using viua::support::tty::COLOR_FG_CYAN;
    using viua::support::tty::COLOR_FG_ORANGE_RED_1;
    using viua::support::tty::COLOR_FG_RED;
    using viua::support::tty::COLOR_FG_RED_1;
    using viua::support::tty::COLOR_FG_WHITE;
    using viua::support::tty::send_escape_seq;
    constexpr auto esc = send_escape_seq;

    auto const args = std::vector<std::string>{(argv + 1), (argv + argc)};
    if (args.empty()) {
        std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                  << ": no file to assemble\n";
        return 1;
    }

    auto preferred_output_path = std::optional<std::filesystem::path>{};
    auto verbosity_level       = 0;
    auto show_version          = false;
    auto show_help             = false;

    for (auto i = decltype(args)::size_type{}; i < args.size(); ++i) {
        auto const& each = args.at(i);
        if (each == "--") {
            // explicit separator of options and operands
            ++i;
            break;
        }
        /*
         * Tool-specific options.
         */
        else if (each == "-o") {
            preferred_output_path = std::filesystem::path{args.at(++i)};
        }
        /*
         * Common options.
         */
        else if (each == "-v" or each == "--verbose") {
            ++verbosity_level;
        } else if (each == "--version") {
            show_version = true;
        } else if (each == "--help") {
            show_help = true;
        } else if (each.front() == '-') {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": unknown option: " << each << "\n";
            return 1;
        } else {
            // input files start here
            ++i;
            break;
        }
    }

    if (show_version) {
        if (verbosity_level) {
            std::cout << "Viua VM ";
        }
        std::cout << (verbosity_level ? VIUAVM_VERSION_FULL : VIUAVM_VERSION)
                  << "\n";
        return 0;
    }
    if (show_help) {
        if (execlp("man", "man", "1", "viua-asm", nullptr) == -1) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": man(1) page not installed or not found\n";
            return 1;
        }
    }

    /*
     * If invoked *with* some arguments, find the path to the source file and
     * assemble it - converting assembly source code into binary. Produced
     * binary may be:
     *
     *  - executable (default): an ELF executable, suitable to be run by Viua VM
     *    kernel
     *  - linkable (with -c flag): an ELF relocatable object file, which should
     *    be linked with other object files to produce a final executable or
     *    shared object
     */
    auto const source_path = std::filesystem::path{args.back()};
    auto source_text       = std::string{};
    {
        auto const source_fd = open(source_path.c_str(), O_RDONLY);
        if (source_fd == -1) {
            using viua::support::tty::ATTR_RESET;
            using viua::support::tty::COLOR_FG_RED;
            using viua::support::tty::COLOR_FG_WHITE;
            using viua::support::tty::send_escape_seq;
            constexpr auto esc = send_escape_seq;

            auto const error_message = viua::support::errno_desc(errno);
            std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                      << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                      << "error" << esc(2, ATTR_RESET) << ": " << error_message
                      << "\n";
            return 1;
        }

        struct stat source_stat {};
        if (fstat(source_fd, &source_stat) == -1) {
            using viua::support::tty::ATTR_RESET;
            using viua::support::tty::COLOR_FG_RED;
            using viua::support::tty::COLOR_FG_WHITE;
            using viua::support::tty::send_escape_seq;
            constexpr auto esc = send_escape_seq;

            auto const error_message = viua::support::errno_desc(errno);
            std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                      << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                      << "error" << esc(2, ATTR_RESET) << ": " << error_message
                      << "\n";
            return 1;
        }
        if (source_stat.st_size == 0) {
            using viua::support::tty::ATTR_RESET;
            using viua::support::tty::COLOR_FG_RED;
            using viua::support::tty::COLOR_FG_WHITE;
            using viua::support::tty::send_escape_seq;
            constexpr auto esc = send_escape_seq;

            std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                      << esc(2, ATTR_RESET) << ": "
                      << esc(2, COLOR_FG_ORANGE_RED_1) << "warning"
                      << esc(2, ATTR_RESET) << ": empty source file\n";
        }

        source_text.resize(source_stat.st_size);
        read(source_fd, source_text.data(), source_text.size());
        close(source_fd);
    }

    auto const output_path = preferred_output_path.value_or(
        [source_path]() -> std::filesystem::path {
            auto o = source_path;
            o.replace_extension("o");
            return o;
        }());

    /*
     * Lexical analysis (lexing).
     *
     * Split the loaded source code into a stream of lexemes for easier
     * processing later. The first point at which errors are detected eg, if
     * illegal characters are used, strings are unclosed, etc.
     */
    auto lexemes = viua::libs::lexer::stage::lex(source_path, source_text);
    if constexpr (false and DEBUG_LEX) {
        std::cerr << lexemes.size() << " raw lexeme(s)\n";
        dump_lexemes(lexemes);
    }

    lexemes = viua::libs::lexer::stage::remove_noise(std::move(lexemes));
    if constexpr (DEBUG_LEX) {
        std::cerr << lexemes.size() << " cooked lexeme(s)\n";
        dump_lexemes(lexemes);
    }

    if (auto e = any_find_mistake(lexemes); e.has_value()) {
        viua::libs::stage::display_error_and_exit(source_path, source_text, *e);
    }

    auto nodes = std::vector<std::unique_ptr<ast::Node>>{};
    try {
        nodes = parse(lexemes);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        viua::libs::stage::display_error_and_exit(source_path, source_text, e);
    }
    if constexpr (DEBUG_PARSE) {
        std::cerr << nodes.size() << " AST nodes(s)\n";
        dump_nodes(nodes);
    }

    auto declared_symbols = std::vector<ast::Symbol>{};
    auto rodata_contents  = std::vector<uint8_t>{};
    auto string_table     = std::vector<uint8_t>{};
    auto symbol_table     = std::vector<Elf64_Sym>{};
    auto symbol_map       = std::map<std::string, size_t>{};
    auto fn_offsets       = std::map<std::string, size_t>{};

    /*
     * ELF standard requires the first byte in the string table to be zero.
     */
    string_table.push_back('\0');

    {
        auto empty     = Elf64_Sym{};
        empty.st_name  = STN_UNDEF;
        empty.st_info  = ELF64_ST_INFO(STB_LOCAL, STT_NOTYPE);
        empty.st_shndx = SHN_UNDEF;
        symbol_table.push_back(empty);

        auto file_sym    = Elf64_Sym{};
        file_sym.st_name = viua::libs::stage::save_string_to_strtab(
            string_table, source_path.native());
        file_sym.st_info  = ELF64_ST_INFO(STB_LOCAL, STT_FILE);
        file_sym.st_shndx = SHN_ABS;
        symbol_table.push_back(file_sym);
    }

    try {
        save_declared_symbols(
            nodes, string_table, symbol_table, symbol_map, declared_symbols);
        cache_function_labels(nodes, string_table, symbol_table, symbol_map);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        viua::libs::stage::display_error_and_exit(source_path, source_text, e);
    }

    try {
        save_objects(
            nodes, rodata_contents, string_table, symbol_table, symbol_map);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        viua::libs::stage::display_error_and_exit(source_path, source_text, e);
    }

    auto text = Text{};
    try {
        text = cook_instructions(
            nodes, rodata_contents, string_table, symbol_table, symbol_map);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        viua::libs::stage::display_error_and_exit(source_path, source_text, e);
    }

#if 0

    stage::load_function_labels(nodes, string_table, symbol_table, symbol_map);
    stage::load_value_labels(source_path,
                             source_text,
                             nodes,
                             rodata_contents,
                             string_table,
                             symbol_table,
                             symbol_map);

    stage::cook_long_immediates(source_path,
                                source_text,
                                nodes,
                                rodata_contents,
                                symbol_table,
                                symbol_map);
#endif

    /*
     * ELF standard requires the last byte in the string table to be zero.
     */
    string_table.push_back('\0');

#if 0
    /*
     * Pseudoinstruction- and macro-expansion.
     *
     * Replace pseudoinstructions (eg, li) with sequences of real instructions
     * that will have the same effect. Ditto for macros.
     */
    stage::cook_pseudoinstructions(source_path, source_text, nodes, symbol_map);
#endif

    /*
     * Detect entry point function.
     *
     * We're not handling relocatable files (shared libs, etc) yet so it makes
     * sense to enforce entry function presence in all cases. Once the
     * relocatables and separate compilation is supported again, this should be
     * hidden behind a flag.
     */
    auto entry_point_fn = std::optional<viua::libs::lexer::Lexeme>{};
    try {
        entry_point_fn = find_entry_point(nodes, symbol_table, symbol_map);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        viua::libs::stage::display_error_and_exit(source_path, source_text, e);
    }
    if (entry_point_fn.has_value()) {
        auto const sym = symbol_table.at(symbol_map.at(entry_point_fn->text));
        std::cerr << "entry point is " << entry_point_fn->text
                  << " at [.text+0x" << std::hex << std::setfill('0')
                  << std::setw(16) << sym.st_value << std::dec
                  << std::setfill(' ') << "]\n";
    }

#if 0
    /*
     * Bytecode emission.
     *
     * This stage is also responsible for preparing the function table. It is a
     * table mapping function names to the offsets inside the .text section, at
     * which their entry points reside.
     */
    auto const text = stage::emit_bytecode(
        source_path, source_text, nodes, symbol_table, symbol_map);
#endif
    auto const reloc_table = make_reloc_table(text);

    for (auto const& decl : declared_symbols) {
        auto const& sym = symbol_table.at(symbol_map.at(decl.name.text));

        auto const was_declared_extern =
            static_cast<bool>(decl.has_attr("extern"));
        auto const was_defined =
            static_cast<bool>(sym.st_value and sym.st_size);

        if (was_declared_extern and was_defined) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            auto e = Error{
                decl.name,
                Cause::None,
                "symbol declared [[extern]], but a definition was provided"};
            viua::libs::stage::display_error_and_exit(
                source_path, source_text, e);
        } else if ((not was_declared_extern) and (not was_defined)) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            auto e = Error{decl.name,
                           Cause::None,
                           "symbol declared, but no definition was provided"};
            e.note("add [[extern]] if the symbol is defined in another "
                   "compilation unit or module");
            viua::libs::stage::display_error_and_exit(
                source_path, source_text, e);
        }
    }

    /*
     * ELF emission.
     */
    emit_elf(
        output_path,
        false,
        (entry_point_fn.has_value()
             ? std::optional{symbol_table
                                 .at(symbol_map.at(entry_point_fn.value().text))
                                 .st_value}
             : std::nullopt),
        text,
        reloc_table,
        rodata_contents,
        string_table,
        symbol_table);

    return 0;
}
