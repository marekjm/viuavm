/*
 *  Copyright (C) 2018 Marek Marecki
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

#include <string>
#include <vector>
#include <viua/assembler/frontend/static_analyser.h>
#include <viua/support/string.h>

using viua::assembler::frontend::parser::Instruction;

namespace viua { namespace assembler { namespace frontend {
namespace static_analyser { namespace checkers {
using viua::assembler::frontend::parser::Atom_literal;
using viua::assembler::frontend::parser::Bits_literal;
using viua::assembler::frontend::parser::Boolean_literal;
using viua::assembler::frontend::parser::Function_name_literal;
using viua::assembler::frontend::parser::Label;
using viua::assembler::frontend::parser::Offset;
using viua::assembler::frontend::parser::Register_index;
using viua::assembler::frontend::parser::Void_literal;
using viua::cg::lex::Invalid_syntax;
using viua::cg::lex::Token;
using viua::cg::lex::Traced_syntax_error;

using viua::internals::Register_sets;
auto register_set_names = std::map<Register_sets, std::string>{
    {
        Register_sets::GLOBAL,
        "global",
    },
    {
        Register_sets::STATIC,
        "static",
    },
    {
        Register_sets::LOCAL,
        "local",
    },
    {
        Register_sets::ARGUMENTS,
        "arguments",
    },
    {
        Register_sets::PARAMETERS,
        "parameters",
    },
};
auto to_string(Register_sets const register_set_id) -> std::string {
    return register_set_names.at(register_set_id);
}

auto invalid_syntax(std::vector<Token> const& tokens, std::string const message)
    -> Invalid_syntax {
    auto invalid_syntax_error = Invalid_syntax(tokens.at(0), message);
    for (auto i = std::remove_reference_t<decltype(tokens)>::size_type{1};
         i < tokens.size();
         ++i) {
        invalid_syntax_error.add(tokens.at(i));
    }
    return invalid_syntax_error;
}

auto get_line_index_of_instruction(InstructionIndex const n,
                                   Instructions_block const& ib)
    -> InstructionIndex {
    auto left = n;
    auto i    = InstructionIndex{0};
    for (; left and i < ib.body.size(); ++i) {
        auto instruction =
            dynamic_cast<viua::assembler::frontend::parser::Instruction*>(
                ib.body.at(i).get());
        if (instruction) {
            --left;
        }
    }
    return i;
}

auto erase_if_direct_access(
    Register_usage_profile& register_usage_profile,
    Register_index* const r,
    viua::assembler::frontend::parser::Instruction const& instruction) -> void {
    if (r->as == viua::internals::Access_specifier::DIRECT) {
        register_usage_profile.erase(Register(*r), instruction.tokens.at(0));
    }
}

template<typename K, typename V>
static auto keys_of(std::map<K, V> const& m) -> std::vector<K> {
    auto keys = std::vector<K>{};

    for (auto const& each : m) {
        keys.push_back(each.first);
    }

    return keys;
}
auto check_if_name_resolved(Register_usage_profile const& rup,
                            Register_index const r) -> void {
    if (not r.resolved) {
        auto error = Invalid_syntax(r.tokens.at(0), "unresolved name");
        if (auto suggestion = str::levenshtein_best(
                r.tokens.at(0).str().substr(1), keys_of(rup.name_to_index), 4);
            suggestion.first) {
            error.aside(
                r.tokens.at(0),
                "did you mean '" + suggestion.second + "' (name of "
                    + std::to_string(rup.name_to_index.at(suggestion.second))
                    + ")?");
        }
        throw error;
    }
}
static auto maybe_mistyped_register_set_helper(
    Register_usage_profile& rup,
    viua::assembler::frontend::parser::Register_index r,
    Traced_syntax_error& error,
    Register_sets rs_id) -> bool {
    if (r.rss != rs_id) {
        auto val         = Register{};
        val.index        = r.index;
        val.register_set = rs_id;
        if (rup.defined(val)) {
            error.errors.back().aside(r.tokens.at(0),
                                      "did you mean " + to_string(rs_id)
                                          + " register "
                                          + std::to_string(r.index) + "?");
            error.append(Invalid_syntax(rup.defined_where(val), "")
                             .note(to_string(rs_id) + " register "
                                   + std::to_string(r.index)
                                   + " was defined here"));
            return true;
        }
    }
    return false;
}
static auto maybe_mistyped_register_set(
    Register_usage_profile& rup,
    viua::assembler::frontend::parser::Register_index r,
    Traced_syntax_error& error) -> void {
    if (maybe_mistyped_register_set_helper(
            rup, r, error, Register_sets::LOCAL)) {
        return;
    }
    if (maybe_mistyped_register_set_helper(
            rup, r, error, Register_sets::STATIC)) {
        return;
    }
}
auto check_use_of_register(Register_usage_profile& rup,
                           viua::assembler::frontend::parser::Register_index r,
                           std::string const error_core_msg,
                           bool const allow_arguments,
                           bool const allow_parameters) -> void {
    check_if_name_resolved(rup, r);
    if (r.rss == Register_sets::GLOBAL) {
        /*
         * Do not check global register set access.
         * There is currently no simple (or complicated) way to check if such
         * accesses are correct or not.
         * FIXME Maybe check global register set accesses?
         */
        return;
    }
    if (r.rss == Register_sets::STATIC) {
        /*
         * Do not check static register set access.
         * There is currently no simple (or complicated) way to check if such
         * accesses are correct or not.
         * FIXME Maybe check static register set accesses?
         */
        return;
    }
    if ((r.rss == Register_sets::ARGUMENTS) and not allow_arguments) {
        throw Traced_syntax_error{}
            .append(Invalid_syntax{
                r.tokens.at(0),
                "invalid use of arguments register set"}
                        .add(r.tokens.at(1))
                        .note("arguments register set may only be used in target register of `copy` and `move` instructions when a frame is allocated"));
    }
    if ((r.rss == Register_sets::PARAMETERS) and not allow_parameters) {
        throw Traced_syntax_error{}
            .append(Invalid_syntax{
                r.tokens.at(0),
                "invalid use of parameters register set"}
                        .add(r.tokens.at(1))
                        .note("parameters register set may only be used in source register of `copy` and `move` instructions"));
    }

    if (not rup.in_bounds(r)) {
        throw Traced_syntax_error{}
            .append(Invalid_syntax{
                r.tokens.at(0),
                ("access to register " + std::to_string(r.index) + " with only "
                 + std::to_string(rup.allocated_registers().value())
                 + " register(s) allocated")}
                        .add(r.tokens.at(1)))
            .append(Invalid_syntax{rup.allocated_where().value(), ""}.note(
                "increase this value to " + std::to_string(r.index + 1)
                + " to fix this issue"));
    }

    if (not rup.defined(Register(r))) {
        auto empty_or_erased = (rup.erased(Register(r)) ? "erased" : "empty");

        auto msg = std::ostringstream{};
        msg << error_core_msg << ' ' << empty_or_erased << ' '
            << to_string(r.rss) << " register "
            << str::enquote(std::to_string(r.index));
        if (rup.index_to_name.count(r.index)) {
            msg << " (named " << str::enquote(rup.index_to_name.at(r.index))
                << ')';
        }
        auto error = Traced_syntax_error{}.append(
            Invalid_syntax(r.tokens.at(0), msg.str()));

        if (rup.erased(Register(r))) {
            error.append(Invalid_syntax(rup.erased_where(Register(r)), "")
                             .note("erased here:"));
        }

        maybe_mistyped_register_set(rup, r, error);

        throw error;
    }
    rup.use(Register(r), r.tokens.at(0));
}

using Value_types           = viua::internals::Value_types;
using ValueTypesType        = viua::internals::ValueTypesType;
auto const value_type_names = std::map<Value_types, std::string>{
    {
        /*
         * Making this "value" instead of "undefined" lets us build error
         * messages for unused registers. They "unused value" can become "unused
         * number" or "unused boolean", and if the SA could not infer the type
         * for the value we will not print a "unused undefined" (what would that
         * even mean?) message, but "unused value".
         */
        Value_types::UNDEFINED,
        "value",
    },
    {
        Value_types::VOID,
        "void",
    },
    {
        Value_types::INTEGER,
        "integer",
    },
    {
        Value_types::FLOAT,
        "float",
    },
    {
        Value_types::NUMBER,
        "number",
    },
    {
        Value_types::BOOLEAN,
        "boolean",
    },
    {
        Value_types::TEXT,
        "text",
    },
    {
        Value_types::STRING,
        "string",
    },
    {
        Value_types::VECTOR,
        "vector",
    },
    {
        Value_types::BITS,
        "bits",
    },
    {
        Value_types::CLOSURE,
        "closure",
    },
    {
        Value_types::FUNCTION,
        "function",
    },
    {
        Value_types::INVOCABLE,
        "invocable",
    },
    {
        Value_types::ATOM,
        "atom",
    },
    {
        Value_types::PID,
        "pid",
    },
    {
        Value_types::STRUCT,
        "struct",
    },
    {
        Value_types::OBJECT,
        "object",
    },
};
auto operator|(const Value_types lhs, const Value_types rhs) -> Value_types {
    // FIXME find out if it is possible to remove the outermost static_cast<>
    return static_cast<Value_types>(static_cast<ValueTypesType>(lhs)
                                    | static_cast<ValueTypesType>(rhs));
}
auto operator&(const Value_types lhs, const Value_types rhs) -> Value_types {
    // FIXME find out if it is possible to remove the outermost static_cast<>
    return static_cast<Value_types>(static_cast<ValueTypesType>(lhs)
                                    & static_cast<ValueTypesType>(rhs));
}
auto operator^(const Value_types lhs, const Value_types rhs) -> Value_types {
    // FIXME find out if it is possible to remove the outermost static_cast<>
    return static_cast<Value_types>(static_cast<ValueTypesType>(lhs)
                                    ^ static_cast<ValueTypesType>(rhs));
}
auto operator!(const Value_types v) -> bool {
    return not static_cast<ValueTypesType>(v);
}
auto to_string(Value_types const value_type_id) -> std::string {
    auto const has_pointer = not not(value_type_id & Value_types::POINTER);
    auto const type_name   = value_type_names.at(
        has_pointer ? (value_type_id ^ Value_types::POINTER) : value_type_id);
    return (has_pointer ? "pointer to " : "") + type_name;
}
auto depointerise_type_if_needed(Value_types const t,
                                 bool const access_via_pointer_dereference)
    -> Value_types {
    return (access_via_pointer_dereference ? (t ^ Value_types::POINTER) : t);
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers
