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
using viua::assembler::frontend::parser::AtomLiteral;
using viua::assembler::frontend::parser::BitsLiteral;
using viua::assembler::frontend::parser::BooleanLiteral;
using viua::assembler::frontend::parser::FunctionNameLiteral;
using viua::assembler::frontend::parser::Label;
using viua::assembler::frontend::parser::Offset;
using viua::assembler::frontend::parser::RegisterIndex;
using viua::assembler::frontend::parser::VoidLiteral;
using viua::cg::lex::InvalidSyntax;
using viua::cg::lex::Token;
using viua::cg::lex::TracedSyntaxError;

using viua::internals::RegisterSets;
auto register_set_names = std::map<RegisterSets, std::string>{
    {
        // FIXME 'current' as a register set name should be deprecated.
        RegisterSets::CURRENT,
        "current",
    },
    {
        RegisterSets::GLOBAL,
        "global",
    },
    {
        RegisterSets::STATIC,
        "static",
    },
    {
        RegisterSets::LOCAL,
        "local",
    },
};
auto to_string(RegisterSets const register_set_id) -> std::string {
    return register_set_names.at(register_set_id);
}

auto invalid_syntax(std::vector<Token> const& tokens, std::string const message)
    -> InvalidSyntax {
    auto invalid_syntax_error = InvalidSyntax(tokens.at(0), message);
    for (auto i = std::remove_reference_t<decltype(tokens)>::size_type{1};
         i < tokens.size();
         ++i) {
        invalid_syntax_error.add(tokens.at(i));
    }
    return invalid_syntax_error;
}

auto get_line_index_of_instruction(InstructionIndex const n,
                                   InstructionsBlock const& ib)
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
    RegisterIndex* const r,
    viua::assembler::frontend::parser::Instruction const& instruction) -> void {
    if (r->as == viua::internals::AccessSpecifier::DIRECT) {
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
                            RegisterIndex const r) -> void {
    if (not r.resolved) {
        auto error = InvalidSyntax(r.tokens.at(0), "unresolved name");
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
    viua::assembler::frontend::parser::RegisterIndex r,
    TracedSyntaxError& error,
    RegisterSets rs_id) -> bool {
    if (r.rss != rs_id) {
        auto val         = Register{};
        val.index        = r.index;
        val.register_set = rs_id;
        if (rup.defined(val)) {
            error.errors.back().aside(r.tokens.at(0),
                                      "did you mean " + to_string(rs_id)
                                          + " register "
                                          + std::to_string(r.index) + "?");
            error.append(InvalidSyntax(rup.defined_where(val), "")
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
    viua::assembler::frontend::parser::RegisterIndex r,
    TracedSyntaxError& error) -> void {
    if (maybe_mistyped_register_set_helper(
            rup, r, error, RegisterSets::LOCAL)) {
        return;
    }
    if (maybe_mistyped_register_set_helper(
            rup, r, error, RegisterSets::STATIC)) {
        return;
    }
}
auto check_use_of_register(Register_usage_profile& rup,
                           viua::assembler::frontend::parser::RegisterIndex r,
                           std::string const error_core_msg) -> void {
    check_if_name_resolved(rup, r);
    if (r.rss == RegisterSets::GLOBAL) {
        /*
         * Do not check global register set access.
         * There is currently no simple (or complicated) way to check if such
         * accesses are correct or not.
         * FIXME Maybe check global register set accesses?
         */
        return;
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
        auto error = TracedSyntaxError{}.append(
            InvalidSyntax(r.tokens.at(0), msg.str()));

        if (rup.erased(Register(r))) {
            error.append(InvalidSyntax(rup.erased_where(Register(r)), "")
                             .note("erased here:"));
        }

        maybe_mistyped_register_set(rup, r, error);

        throw error;
    }
    rup.use(Register(r), r.tokens.at(0));
}

using ValueTypes            = viua::internals::ValueTypes;
using ValueTypesType        = viua::internals::ValueTypesType;
auto const value_type_names = std::map<ValueTypes, std::string>{
    {
        /*
         * Making this "value" instead of "undefined" lets us build error
         * messages for unused registers. They "unused value" can become "unused
         * number" or "unused boolean", and if the SA could not infer the type
         * for the value we will not print a "unused undefined" (what would that
         * even mean?) message, but "unused value".
         */
        ValueTypes::UNDEFINED,
        "value",
    },
    {
        ValueTypes::VOID,
        "void",
    },
    {
        ValueTypes::INTEGER,
        "integer",
    },
    {
        ValueTypes::FLOAT,
        "float",
    },
    {
        ValueTypes::NUMBER,
        "number",
    },
    {
        ValueTypes::BOOLEAN,
        "boolean",
    },
    {
        ValueTypes::TEXT,
        "text",
    },
    {
        ValueTypes::STRING,
        "string",
    },
    {
        ValueTypes::VECTOR,
        "vector",
    },
    {
        ValueTypes::BITS,
        "bits",
    },
    {
        ValueTypes::CLOSURE,
        "closure",
    },
    {
        ValueTypes::FUNCTION,
        "function",
    },
    {
        ValueTypes::INVOCABLE,
        "invocable",
    },
    {
        ValueTypes::ATOM,
        "atom",
    },
    {
        ValueTypes::PID,
        "pid",
    },
    {
        ValueTypes::STRUCT,
        "struct",
    },
    {
        ValueTypes::OBJECT,
        "object",
    },
};
auto operator|(const ValueTypes lhs, const ValueTypes rhs) -> ValueTypes {
    // FIXME find out if it is possible to remove the outermost static_cast<>
    return static_cast<ValueTypes>(static_cast<ValueTypesType>(lhs)
                                   | static_cast<ValueTypesType>(rhs));
}
auto operator&(const ValueTypes lhs, const ValueTypes rhs) -> ValueTypes {
    // FIXME find out if it is possible to remove the outermost static_cast<>
    return static_cast<ValueTypes>(static_cast<ValueTypesType>(lhs)
                                   & static_cast<ValueTypesType>(rhs));
}
auto operator^(const ValueTypes lhs, const ValueTypes rhs) -> ValueTypes {
    // FIXME find out if it is possible to remove the outermost static_cast<>
    return static_cast<ValueTypes>(static_cast<ValueTypesType>(lhs)
                                   ^ static_cast<ValueTypesType>(rhs));
}
auto operator!(const ValueTypes v) -> bool {
    return not static_cast<ValueTypesType>(v);
}
auto to_string(ValueTypes const value_type_id) -> std::string {
    auto const has_pointer = not not(value_type_id & ValueTypes::POINTER);
    auto const type_name   = value_type_names.at(
        has_pointer ? (value_type_id ^ ValueTypes::POINTER) : value_type_id);
    return (has_pointer ? "pointer to " : "") + type_name;
}
auto depointerise_type_if_needed(ValueTypes const t,
                                 bool const access_via_pointer_dereference)
    -> ValueTypes {
    return (access_via_pointer_dereference ? (t ^ ValueTypes::POINTER) : t);
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers
