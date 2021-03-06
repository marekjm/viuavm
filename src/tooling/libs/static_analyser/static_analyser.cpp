/*
 *  Copyright (C) 2018-2019 Marek Marecki
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

#include <initializer_list>
#include <iostream>
#include <optional>
#include <string>

#include <viua/tooling/errors/compile_time/errors.h>
#include <viua/tooling/libs/static_analyser/static_analyser.h>

namespace viua {
    namespace tooling {
        namespace libs {
            namespace static_analyser {
static auto to_string(viua::bytecode::codec::Register_set const rs)
    -> std::string
{
    using viua::bytecode::codec::Register_set;
    switch (rs) {
    case viua::bytecode::codec::Register_set::Global:
        return "global";
    case viua::bytecode::codec::Register_set::Local:
        return "local";
    case viua::bytecode::codec::Register_set::Static:
        return "static";
    case viua::bytecode::codec::Register_set::Arguments:
        return "arguments";
    case viua::bytecode::codec::Register_set::Parameters:
        return "parameters";
    case viua::bytecode::codec::Register_set::Closure_local:
        return "closure_local";
    default:
        return "<unknown>";
    }
}

static auto to_string(std::vector<values::Value_type> const& value,
                      std::vector<values::Value_type>::size_type const i)
    -> std::string
{
    switch (value.at(i)) {
    case values::Value_type::Value:
        return "value";
    case values::Value_type::Integer:
        return "integer";
    case values::Value_type::Float:
        return "float";
    case values::Value_type::Vector:
        return "vector of " + to_string(value, i + 1);
    case values::Value_type::String:
        return "string";
    case values::Value_type::Text:
        return "text";
    case values::Value_type::Pointer:
        return "pointer to " + to_string(value, i + 1);
    case values::Value_type::Boolean:
        return "boolean";
    case values::Value_type::Bits:
        return "bits";
    case values::Value_type::Closure:
        return "closure";
    case values::Value_type::Function:
        return "function";
    case values::Value_type::Atom:
        return "atom";
    case values::Value_type::Struct:
        return "struct";
    case values::Value_type::Pid:
        return "pid";
    default:
        return "value";
    }
}
static auto to_string(std::vector<values::Value_type> const& value)
    -> std::string
{
    return to_string(value, 0);
}

template<typename Container, typename Appender>
auto copy_whole(Container const& source, Appender const& appender) -> void
{
    std::copy(source.begin(), source.end(), appender);
}

static auto maybe_with_pointer(
    viua::internals::Access_specifier const access,
    std::initializer_list<values::Value_type> const base)
    -> std::vector<values::Value_type>
{
    auto signature =
        (access == viua::internals::Access_specifier::POINTER_DEREFERENCE
             ? std::vector<values::Value_type>{values::Value_type::Pointer}
             : std::vector<values::Value_type>{});
    copy_whole(base, std::back_inserter(signature));
    return signature;
}
static auto maybe_with_pointer(viua::internals::Access_specifier const access,
                               std::vector<values::Value_type> const base)
    -> std::vector<values::Value_type>
{
    auto signature =
        (access == viua::internals::Access_specifier::POINTER_DEREFERENCE
             ? std::vector<values::Value_type>{values::Value_type::Pointer}
             : std::vector<values::Value_type>{});
    copy_whole(base, std::back_inserter(signature));
    return signature;
}

static auto throw_if_empty(
    Function_state& function_state,
    viua::tooling::libs::parser::Register_address const& address)
    -> viua::bytecode::codec::register_index_type
{
    auto const address_index = function_state.resolve_index(address);
    if (not function_state.defined(address_index, address.register_set)) {
        auto error =
            viua::tooling::errors::compile_time::Error_wrapper{}.append(
                viua::tooling::errors::compile_time::Error{
                    viua::tooling::errors::compile_time::Compile_time_error::
                        Read_from_empty_register,
                    address.tokens().at(1),
                    std::to_string(address_index)}
                    .add(address.tokens().at(2)));
        if (function_state.erased(address_index, address.register_set)) {
            auto const& erased_location =
                function_state.erased_at(address_index, address.register_set);
            auto partial =
                viua::tooling::errors::compile_time::Error{
                    viua::tooling::errors::compile_time::Compile_time_error::
                        Empty_error,
                    erased_location.at(0)}
                    .note("erased here");
            for (auto each = erased_location.begin() + 1;
                 each != erased_location.end();
                 ++each) {
                partial.add(*each);
            }
            error.append(partial);
        }
        throw error;
    }
    return address_index;
}

static auto throw_if_invalid_type(
    Function_state& function_state,
    viua::tooling::libs::parser::Register_address const& address,
    viua::bytecode::codec::register_index_type const index,
    std::vector<values::Value_type> const type_signature) -> void
{
    if (not function_state.assume_type(
            index, address.register_set, type_signature)) {
        auto error =
            viua::tooling::errors::compile_time::Error_wrapper{}.append(
                viua::tooling::errors::compile_time::Error{
                    viua::tooling::errors::compile_time::Compile_time_error::
                        Type_mismatch,
                    address.tokens().at(0),
                    "expected `" + to_string(type_signature) + "'..."}
                    .add(address.tokens().at(0)));

        auto const& definition_location =
            function_state.defined_at(index, address.register_set);
        error.append(viua::tooling::errors::compile_time::Error{
            viua::tooling::errors::compile_time::Compile_time_error::
                Empty_error,
            definition_location.at(0),
            ("...got `"
             + to_string(function_state.type_of(index, address.register_set)
                             .to_simple())
             + "'")}
                         .note("defined here"));
        throw error;
    }
}

template<typename T>
static auto prepend(T&& element, std::vector<T> const& seq) -> std::vector<T>
{
    auto v = std::vector<T>{};

    v.reserve(seq.size() + 1);
    v.push_back(element);
    copy_whole(seq, std::back_inserter(v));

    return v;
}

template<typename T> auto int_range(T const n) -> std::vector<T>
{
    auto v = std::vector<T>{};
    v.reserve(n);

    for (auto i = typename decltype(v)::size_type{0}; i < n; ++i) {
        v.emplace_back(i);
    }

    return v;
}

struct Frame_representation {
    viua::bytecode::codec::register_index_type const allocated_parameters;
    std::map<viua::bytecode::codec::register_index_type,
             viua::tooling::libs::lexer::Token>
        filled_parameters;

    Frame_representation(viua::bytecode::codec::register_index_type const);
};
Frame_representation::Frame_representation(
    viua::bytecode::codec::register_index_type const size)
        : allocated_parameters{size}
{}

struct Body_line {
    using size_type = decltype(
        viua::tooling::libs::parser::Cooked_function::lines)::size_type;

    size_type const source_line{0};
    size_type const instruction{0};

    Body_line(size_type const s, size_type const i)
            : source_line{s}, instruction{i}
    {}
    Body_line(Body_line const&) = default;
    Body_line(Body_line&&)      = default;
    auto operator=(Body_line const&) -> Body_line& = delete;
    auto operator=(Body_line &&) -> Body_line& = delete;
};

static auto annotate_body(
    viua::tooling::libs::parser::Cooked_function::body_type body)
    -> std::vector<Body_line>
{
    auto annotated_body = std::vector<Body_line>{};

    auto source_line = Body_line::size_type{0};
    auto instruction = Body_line::size_type{0};

    for (auto const& each : body) {
        annotated_body.emplace_back(Body_line{source_line, instruction});

        using viua::tooling::libs::parser::Fragment_type;
        if (each->type() == Fragment_type::Instruction) {
            ++instruction;
        }
        ++source_line;
    }

    return annotated_body;
}

static auto create_label_map(
    viua::tooling::libs::parser::Cooked_function::body_type const& body,
    std::vector<Body_line> const& annotated_body)
    -> std::map<std::string, Body_line>
{
    auto mapping = std::map<std::string, Body_line>{};

    for (auto i = Body_line::size_type{0}; i < body.size(); ++i) {
        using viua::tooling::libs::parser::Fragment_type;

        auto const line = body.at(i);

        if (line->type() == Fragment_type::Mark_directive) {
            mapping.emplace(line->tokens().at(1).str(), annotated_body.at(i));
        }
    }

    return mapping;
}

struct Arm_result {
    Body_line::size_type last_instruction{0};
    std::set<Body_line::size_type> analysed_lines;
};

static auto analyse_single_arm(
    std::optional<std::reference_wrapper<
        viua::tooling::libs::parser::Cooked_function const>> fn,
    std::optional<
        std::reference_wrapper<viua::tooling::libs::parser::Cooked_block const>>
        bl,
    viua::tooling::libs::parser::Cooked_fragments const& fragments,
    Analyser_state& analyser_state,
    Function_state& function_state,
    bool const after_conditional_branch,
    std::vector<Body_line> const& annotated_body,
    std::map<std::string, Body_line> const& label_map,
    viua::tooling::libs::parser::Cooked_function::body_type::size_type i,
    bool const analyse_all = true) -> Arm_result
{
    using viua::tooling::errors::compile_time::Compile_time_error;
    using viua::tooling::libs::parser::Fragment_type;
    using viua::tooling::libs::parser::Instruction;
    using viua::tooling::libs::parser::Operand_type;
    using viua::tooling::libs::parser::Register_address;

    auto const analysed_region_name =
        (fn.has_value() ? (fn.value().get().head().function_name + '/'
                           + std::to_string(fn.value().get().head().arity))
                        : bl.value().get().body().at(0)->tokens().at(0).str());

    auto arm_result = Arm_result{};

    /*
     * There may be no branches between a frame spawn point and the call
     * instruction that follows it. The only allowed instructions
     * between these two points are `move` and `copy` instructions that
     * have the arguments register set as destination.
     */
    auto spawned_frame       = std::unique_ptr<Frame_representation>();
    auto spawned_frame_where = viua::tooling::libs::lexer::Token{};

    auto spawned_catch_frame       = false;
    auto spawned_catch_frame_where = viua::tooling::libs::lexer::Token{};

    for (; i < annotated_body.size(); ++i) {
        arm_result.analysed_lines.insert(i);
        auto const line =
            (bl.has_value()
                 ? bl.value().get().body().at(annotated_body.at(i).source_line)
                 : fn.value().get().body().at(
                     annotated_body.at(i).source_line));

        std::cout << "analysing: " << line->token(0).str() << " (" << i
                  << " of " << (annotated_body.size() - 1) << " lines)"
                  << std::endl;

        if (line->type() == Fragment_type::Name_directive) {
            using viua::tooling::libs::parser::Name_directive;
            auto const& directive = *static_cast<Name_directive const*>(line);
            auto const index =
                (directive.iota ? function_state.iota(directive.token(1))
                                : directive.register_index);

            function_state.rename_register(index, directive.name, directive);
        }

        if (line->type() == Fragment_type::Instruction
            and (annotated_body.at(i).instruction or analyse_all)) {
            auto const& instruction = *static_cast<Instruction const*>(line);

            switch (instruction.opcode) {
            case NOP:
            {
                break;
            }
            case IZERO:
            {
                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                function_state.define_register(
                    function_state.resolve_index(dest),
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Integer>(0)),
                    std::move(defining_tokens));

                break;
            }
            case INTEGER:
            {
                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                using viua::tooling::libs::parser::Integer_literal;
                function_state.define_register(
                    function_state.resolve_index(dest),
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Integer>(
                            static_cast<Integer_literal const*>(
                                instruction.operands.at(1).get())
                                ->n)),
                    std::move(defining_tokens));

                break;
            }
            case IINC:
            case IDEC:
            {
                auto const& target = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto const target_index =
                    throw_if_empty(function_state, target);

                auto const target_access = target.access;
                auto const target_type_signature =
                    (target_access
                     == viua::internals::Access_specifier::POINTER_DEREFERENCE)
                        ? std::vector<
                            values::Value_type>{values::Value_type::Pointer,
                                                values::Value_type::Integer}
                        : std::vector<values::Value_type>{
                            values::Value_type::Integer};
                if (not function_state.assume_type(target_index,
                                                   target.register_set,
                                                   target_type_signature)) {
                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::Type_mismatch,
                                target.tokens().at(0),
                                "expected `" + to_string(target_type_signature)
                                    + "'..."}
                                        .add(target.tokens().at(0)));

                    auto const& definition_location = function_state.defined_at(
                        target_index, target.register_set);
                    error.append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Empty_error,
                        definition_location.at(0),
                        ("...got `"
                         + to_string(
                             function_state
                                 .type_of(target_index, target.register_set)
                                 .to_simple())
                         + "'")}
                                     .note("defined here"));
                    throw error;
                }

                using values::Integer;
                auto& target_operand = static_cast<Integer&>(
                    function_state.type_of(target_index, target.register_set)
                        .value());
                if (target_operand.known()) {
                    if (instruction.opcode == IINC) {
                        target_operand.of(target_operand.of() + 1);
                    } else {
                        target_operand.of(target_operand.of() - 1);
                    }
                }

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(target.tokens(),
                           std::back_inserter(defining_tokens));
                function_state.mutate_register(target_index,
                                               target.register_set,
                                               std::move(defining_tokens));

                break;
            }
            case FLOAT:
            {
                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                using viua::tooling::libs::parser::Integer_literal;
                function_state.define_register(
                    function_state.resolve_index(dest),
                    dest.register_set,
                    function_state.make_wrapper(std::make_unique<values::Float>(
                        /* static_cast<Integer_literal
                           const*>(instruction.operands.at(1).get())->n */
                        )),
                    std::move(defining_tokens));

                break;
            }
            case ITOF:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature =
                    (source.access
                     == viua::internals::Access_specifier::POINTER_DEREFERENCE)
                        ? std::vector<
                            values::Value_type>{values::Value_type::Pointer,
                                                values::Value_type::Integer}
                        : std::vector<values::Value_type>{
                            values::Value_type::Integer};
                if (not function_state.assume_type(source_index,
                                                   source.register_set,
                                                   source_type_signature)) {
                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::Type_mismatch,
                                source.tokens().at(0),
                                "expected `" + to_string(source_type_signature)
                                    + "'..."}
                                        .add(source.tokens().at(0)));

                    auto const& definition_location = function_state.defined_at(
                        source_index, source.register_set);
                    error.append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Empty_error,
                        definition_location.at(0),
                        ("...got `"
                         + to_string(
                             function_state
                                 .type_of(source_index, source.register_set)
                                 .to_simple())
                         + "'")}
                                     .note("defined here"));
                    throw error;
                }

                auto const& destination = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(destination.tokens(),
                           std::back_inserter(defining_tokens));

                auto const destination_index =
                    function_state.resolve_index(destination);
                function_state.define_register(
                    destination_index,
                    destination.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Float>()),
                    std::move(defining_tokens));

                break;
            }
            case FTOI:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature =
                    (source.access
                     == viua::internals::Access_specifier::POINTER_DEREFERENCE)
                        ? std::vector<
                            values::Value_type>{values::Value_type::Pointer,
                                                values::Value_type::Float}
                        : std::vector<values::Value_type>{
                            values::Value_type::Float};
                if (not function_state.assume_type(source_index,
                                                   source.register_set,
                                                   source_type_signature)) {
                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::Type_mismatch,
                                source.tokens().at(0),
                                "expected `" + to_string(source_type_signature)
                                    + "'..."}
                                        .add(source.tokens().at(0)));

                    auto const& definition_location = function_state.defined_at(
                        source_index, source.register_set);
                    error.append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Empty_error,
                        definition_location.at(0),
                        ("...got `"
                         + to_string(
                             function_state
                                 .type_of(source_index, source.register_set)
                                 .to_simple())
                         + "'")}
                                     .note("defined here"));
                    throw error;
                }

                auto const& destination = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(destination.tokens(),
                           std::back_inserter(defining_tokens));

                auto const destination_index =
                    function_state.resolve_index(destination);
                function_state.define_register(
                    destination_index,
                    destination.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Integer>()),
                    std::move(defining_tokens));

                break;
            }
            case STOI:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature =
                    (source.access
                     == viua::internals::Access_specifier::POINTER_DEREFERENCE)
                        ? std::vector<
                            values::Value_type>{values::Value_type::Pointer,
                                                values::Value_type::String}
                        : std::vector<values::Value_type>{
                            values::Value_type::String};
                if (not function_state.assume_type(source_index,
                                                   source.register_set,
                                                   source_type_signature)) {
                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::Type_mismatch,
                                source.tokens().at(0),
                                "expected `" + to_string(source_type_signature)
                                    + "'..."}
                                        .add(source.tokens().at(0)));

                    auto const& definition_location = function_state.defined_at(
                        source_index, source.register_set);
                    error.append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Empty_error,
                        definition_location.at(0),
                        ("...got `"
                         + to_string(
                             function_state
                                 .type_of(source_index, source.register_set)
                                 .to_simple())
                         + "'")}
                                     .note("defined here"));
                    throw error;
                }

                auto const& destination = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(destination.tokens(),
                           std::back_inserter(defining_tokens));

                auto const destination_index =
                    function_state.resolve_index(destination);
                function_state.define_register(
                    destination_index,
                    destination.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Integer>()),
                    std::move(defining_tokens));

                break;
            }
            case STOF:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature =
                    (source.access
                     == viua::internals::Access_specifier::POINTER_DEREFERENCE)
                        ? std::vector<
                            values::Value_type>{values::Value_type::Pointer,
                                                values::Value_type::String}
                        : std::vector<values::Value_type>{
                            values::Value_type::String};
                if (not function_state.assume_type(source_index,
                                                   source.register_set,
                                                   source_type_signature)) {
                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::Type_mismatch,
                                source.tokens().at(0),
                                "expected `" + to_string(source_type_signature)
                                    + "'..."}
                                        .add(source.tokens().at(0)));

                    auto const& definition_location = function_state.defined_at(
                        source_index, source.register_set);
                    error.append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Empty_error,
                        definition_location.at(0),
                        ("...got `"
                         + to_string(
                             function_state
                                 .type_of(source_index, source.register_set)
                                 .to_simple())
                         + "'")}
                                     .note("defined here"));
                    throw error;
                }

                auto const& destination = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(destination.tokens(),
                           std::back_inserter(defining_tokens));

                auto const destination_index =
                    function_state.resolve_index(destination);
                function_state.define_register(
                    destination_index,
                    destination.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Float>()),
                    std::move(defining_tokens));

                break;
            }
            case BITS_OF_INTEGER:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Integer});
                if (not function_state.assume_type(source_index,
                                                   source.register_set,
                                                   source_type_signature)) {
                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::Type_mismatch,
                                source.tokens().at(0),
                                "expected `" + to_string(source_type_signature)
                                    + "'..."}
                                        .add(source.tokens().at(0)));

                    auto const& definition_location = function_state.defined_at(
                        source_index, source.register_set);
                    error.append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Empty_error,
                        definition_location.at(0),
                        ("...got `"
                         + to_string(
                             function_state
                                 .type_of(source_index, source.register_set)
                                 .to_simple())
                         + "'")}
                                     .note("defined here"));
                    throw error;
                }

                auto const& destination = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(destination.tokens(),
                           std::back_inserter(defining_tokens));

                auto const destination_index =
                    function_state.resolve_index(destination);
                function_state.define_register(
                    destination_index,
                    destination.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Bits>()),
                    std::move(defining_tokens));

                break;
            }
            case INTEGER_OF_BITS:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Bits});
                if (not function_state.assume_type(source_index,
                                                   source.register_set,
                                                   source_type_signature)) {
                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::Type_mismatch,
                                source.tokens().at(0),
                                "expected `" + to_string(source_type_signature)
                                    + "'..."}
                                        .add(source.tokens().at(0)));

                    auto const& definition_location = function_state.defined_at(
                        source_index, source.register_set);
                    error.append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Empty_error,
                        definition_location.at(0),
                        ("...got `"
                         + to_string(
                             function_state
                                 .type_of(source_index, source.register_set)
                                 .to_simple())
                         + "'")}
                                     .note("defined here"));
                    throw error;
                }

                auto const& destination = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(destination.tokens(),
                           std::back_inserter(defining_tokens));

                auto const destination_index =
                    function_state.resolve_index(destination);
                function_state.define_register(
                    destination_index,
                    destination.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Integer>()),
                    std::move(defining_tokens));

                break;
            }
            case ADD:
            case SUB:
            case MUL:
            case DIV:
            {
                auto const& lhs = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const& rhs = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());

                auto const lhs_index = throw_if_empty(function_state, lhs);
                auto const lhs_type_signature =
                    (lhs.access
                     == viua::internals::Access_specifier::POINTER_DEREFERENCE)
                        ? std::vector<
                            values::Value_type>{values::Value_type::Pointer,
                                                values::Value_type::Integer}
                        : std::vector<values::Value_type>{
                            values::Value_type::Integer};
                throw_if_invalid_type(
                    function_state, lhs, lhs_index, lhs_type_signature);

                auto const rhs_index = throw_if_empty(function_state, rhs);
                auto const rhs_type_signature =
                    (rhs.access
                     == viua::internals::Access_specifier::POINTER_DEREFERENCE)
                        ? std::vector<
                            values::Value_type>{values::Value_type::Pointer,
                                                values::Value_type::Integer}
                        : std::vector<values::Value_type>{
                            values::Value_type::Integer};
                throw_if_invalid_type(
                    function_state, rhs, rhs_index, rhs_type_signature);

                {
                    using values::Integer;
                    auto const& rhs_operand = static_cast<Integer&>(
                        function_state.type_of(rhs_index, rhs.register_set)
                            .value());
                    if (rhs_operand.known() and rhs_operand.of() == 0) {
                        auto msg = std::ostringstream{};
                        msg << "right-hand side will always be 0, triggering a "
                               "division by zero";

                        auto error =
                            viua::tooling::errors::compile_time::Error_wrapper{}
                                .append(
                                    viua::tooling::errors::compile_time::Error{
                                        viua::tooling::errors::compile_time::
                                            Compile_time_error::Empty_error,
                                        line->tokens().at(0),
                                        msg.str()});

                        {
                            auto const& definition_location =
                                function_state.defined_at(rhs_index,
                                                          rhs.register_set);
                            error.append(
                                viua::tooling::errors::compile_time::Error{
                                    viua::tooling::errors::compile_time::
                                        Compile_time_error::Empty_error,
                                    definition_location.at(0)}
                                    .note("right-hand side defined here:"));
                            if (function_state.mutated(rhs_index,
                                                       rhs.register_set)) {
                                for (auto const& each :
                                     function_state.mutated_at(
                                         rhs_index, rhs.register_set)) {
                                    auto e =
                                        viua::tooling::errors::compile_time::
                                            Error{viua::tooling::errors::
                                                      compile_time::
                                                          Compile_time_error::
                                                              Empty_error,
                                                  each.at(0)};
                                    for (auto it = each.begin() + 1;
                                         it != each.end();
                                         ++it) {
                                        e.add(*it);
                                    }
                                    error.append(e.note(
                                        "right-hand side mutated here:"));
                                }
                            }
                        }

                        throw error;
                    }
                }

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());
                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Integer>()),
                    std::move(defining_tokens));

                break;
            }
            case TEXT:
            {
                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                using viua::tooling::libs::parser::Integer_literal;
                function_state.define_register(
                    function_state.resolve_index(dest),
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Text>()),
                    std::move(defining_tokens));

                break;
            }
            case MOVE:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Value});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const strip_pointer =
                    [](values::Value_wrapper wrapper) -> values::Value_wrapper {
                    if (wrapper.value().type() == values::Value_type::Pointer) {
                        return static_cast<values::Pointer const&>(
                                   wrapper.value())
                            .of();
                    } else {
                        return wrapper;
                    }
                };

                auto const dest_index = function_state.resolve_index(dest);

                if (dest.register_set
                    == viua::bytecode::codec::Register_set::Arguments) {
                    if (not spawned_frame) {
                        auto error =
                            viua::tooling::errors::compile_time::Error_wrapper{}
                                .append(
                                    viua::tooling::errors::compile_time::Error{
                                        Compile_time_error::
                                            Argument_pass_without_a_frame,
                                        instruction.token(0)}
                                        .add(dest.tokens().at(2))
                                        .comment(
                                            "`arguments' register set is only "
                                            "valid when there "
                                            "is an unused frame available"));
                        throw error;
                    }
                    if (spawned_frame->filled_parameters.count(dest.index)) {
                        auto error =
                            viua::tooling::errors::compile_time::Error_wrapper{}
                                .append(
                                    viua::tooling::errors::compile_time::Error{
                                        Compile_time_error::
                                            Argument_pass_overwrites,
                                        instruction.token(0)}
                                        .add(dest.tokens().at(2)))
                                .append(
                                    viua::tooling::errors::compile_time::Error{
                                        Compile_time_error::Empty_error,
                                        spawned_frame->filled_parameters.at(
                                            dest.index)}
                                        .note(
                                            "argument previously passed here"));
                        throw error;
                    }
                    spawned_frame->filled_parameters.emplace(
                        dest.index, instruction.token(0));
                } else {
                    function_state.define_register(
                        dest_index,
                        dest.register_set,
                        strip_pointer(function_state.type_of(
                            source_index, source.register_set)),
                        std::move(defining_tokens));
                }

                if (source.access
                    != viua::internals::Access_specifier::POINTER_DEREFERENCE) {
                    function_state.erase_register(
                        source_index,
                        source.register_set,
                        [&instruction, &source]()
                            -> std::vector<viua::tooling::libs::lexer::Token> {
                            auto tokens = std::vector<
                                viua::tooling::libs::lexer::Token>{};
                            tokens.push_back(instruction.tokens().at(0));
                            copy_whole(source.tokens(),
                                       std::back_inserter(tokens));
                            return tokens;
                        }());
                }

                break;
            }
            case LT:
            case LTE:
            case GT:
            case GTE:
            case EQ:
            {
                auto const& lhs = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const& rhs = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());

                auto const lhs_index = throw_if_empty(function_state, lhs);
                auto const lhs_type_signature = maybe_with_pointer(
                    lhs.access, {values::Value_type::Integer});
                throw_if_invalid_type(
                    function_state, lhs, lhs_index, lhs_type_signature);

                auto const rhs_index = throw_if_empty(function_state, rhs);
                auto const rhs_type_signature = maybe_with_pointer(
                    rhs.access, {values::Value_type::Integer});
                throw_if_invalid_type(
                    function_state, rhs, rhs_index, rhs_type_signature);

                using values::Integer;
                auto const& lhs_operand = static_cast<Integer&>(
                    function_state.type_of(lhs_index, lhs.register_set)
                        .value());
                auto const& rhs_operand = static_cast<Integer&>(
                    function_state.type_of(rhs_index, rhs.register_set)
                        .value());
                if (lhs_operand.known() and rhs_operand.known()) {
                    auto msg = std::ostringstream{};
                    msg << "left-hand side will always be ";
                    auto const l = lhs_operand.of();
                    auto const r = rhs_operand.of();
                    if (l < r) {
                        msg << "less than";
                    } else if (l > r) {
                        msg << "greater than";
                    } else {
                        msg << "equal to";
                    }
                    msg << " right-hand side";

                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::Useless_comparison,
                                line->tokens().at(0),
                                msg.str()});

                    {
                        auto const& definition_location =
                            function_state.defined_at(lhs_index,
                                                      lhs.register_set);
                        error.append(viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::
                                Compile_time_error::Empty_error,
                            definition_location.at(0)}
                                         .note("left-hand side defined here:"));
                    }
                    {
                        auto const& definition_location =
                            function_state.defined_at(rhs_index,
                                                      rhs.register_set);
                        error.append(
                            viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::Empty_error,
                                definition_location.at(0)}
                                .note("right-hand side defined here:"));
                        if (function_state.mutated(rhs_index,
                                                   rhs.register_set)) {
                            for (auto const& each : function_state.mutated_at(
                                     rhs_index, rhs.register_set)) {
                                auto e =
                                    viua::tooling::errors::compile_time::Error{
                                        viua::tooling::errors::compile_time::
                                            Compile_time_error::Empty_error,
                                        each.at(0)};
                                for (auto it = each.begin() + 1;
                                     it != each.end();
                                     ++it) {
                                    e.add(*it);
                                }
                                error.append(
                                    e.note("right-hand side mutated here:"));
                            }
                        }
                    }

                    throw error;
                }

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());
                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Boolean>()),
                    std::move(defining_tokens));

                break;
            }
            case STRING:
            {
                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));

                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                function_state.define_register(
                    function_state.resolve_index(dest),
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::String>()),
                    std::move(defining_tokens));

                break;
            }
            case STREQ:
            case TEXTEQ:
            {
                auto const& lhs = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const& rhs = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());

                auto const lhs_index = throw_if_empty(function_state, lhs);
                auto const lhs_type_signature =
                    maybe_with_pointer(lhs.access, {values::Value_type::Text});
                throw_if_invalid_type(
                    function_state, lhs, lhs_index, lhs_type_signature);

                auto const rhs_index = throw_if_empty(function_state, rhs);
                auto const rhs_type_signature =
                    maybe_with_pointer(rhs.access, {values::Value_type::Text});
                throw_if_invalid_type(
                    function_state, rhs, rhs_index, rhs_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());
                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Boolean>()),
                    std::move(defining_tokens));

                break;
            }
            case TEXTAT:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const& index = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Text});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const index_index = throw_if_empty(function_state, index);
                auto const index_type_signature =
                    std::vector<values::Value_type>{
                        values::Value_type::Integer};
                throw_if_invalid_type(
                    function_state, index, index_index, index_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());
                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Text>()),
                    std::move(defining_tokens));

                break;
            }
            case TEXTSUB:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const& from = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());
                auto const& to = *static_cast<Register_address const*>(
                    instruction.operands.at(3).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Text});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const from_index = throw_if_empty(function_state, from);
                auto const from_type_signature = maybe_with_pointer(
                    from.access, {values::Value_type::Integer});
                throw_if_invalid_type(
                    function_state, from, from_index, from_type_signature);

                auto const to_index = throw_if_empty(function_state, to);
                auto const to_type_signature = maybe_with_pointer(
                    to.access, {values::Value_type::Integer});
                throw_if_invalid_type(
                    function_state, to, to_index, to_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                function_state.define_register(
                    function_state.resolve_index(dest),
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Integer>(
                            // FIXME use length of the text
                            )),
                    std::move(defining_tokens));

                break;
            }
            case TEXTLENGTH:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Text});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Integer>(
                            // FIXME use length of the text
                            )),
                    std::move(defining_tokens));

                break;
            }
            case TEXTCOMMONPREFIX:
            {
                // FIXME TODO
                break;
            }
            case TEXTCOMMONSUFFIX:
            {
                // FIXME TODO
                break;
            }
            case TEXTCONCAT:
            {
                auto const& lhs = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());

                auto const lhs_index = throw_if_empty(function_state, lhs);
                auto const lhs_type_signature =
                    maybe_with_pointer(lhs.access, {values::Value_type::Text});
                throw_if_invalid_type(
                    function_state, lhs, lhs_index, lhs_type_signature);

                auto const& rhs = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());

                auto const rhs_index = throw_if_empty(function_state, rhs);
                auto const rhs_type_signature =
                    maybe_with_pointer(rhs.access, {values::Value_type::Text});
                throw_if_invalid_type(
                    function_state, rhs, rhs_index, rhs_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Integer>(
                            // FIXME use length of the text
                            )),
                    std::move(defining_tokens));

                break;
            }
            case VECTOR:
            {
                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                using viua::tooling::libs::parser::Operand_type;
                if (instruction.operands.at(1)->type() == Operand_type::Void
                    and instruction.operands.at(2)->type()
                            != Operand_type::Void) {
                    throw viua::tooling::errors::compile_time::Error_wrapper{}
                        .append(viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::
                                Compile_time_error::Empty_error,
                            instruction.operands.at(1)->tokens().at(0),
                            "begin-packing operand cannot be void if the "
                            "end-packing operand is not"})
                        .append(
                            viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::Empty_error,
                                instruction.operands.at(2)->tokens().at(0)}
                                .note("end-packing operand is here")
                                .add(instruction.operands.at(2)->tokens().at(1))
                                .add(instruction.operands.at(2)->tokens().at(
                                    2)));
                }

                if (instruction.operands.at(1)->type() == Operand_type::Void) {
                    auto defining_tokens =
                        std::vector<viua::tooling::libs::lexer::Token>{};
                    defining_tokens.push_back(line->token(0));
                    copy_whole(dest.tokens(),
                               std::back_inserter(defining_tokens));

                    auto const dest_index = function_state.resolve_index(dest);
                    function_state.define_register(
                        dest_index,
                        dest.register_set,
                        function_state.make_wrapper(
                            std::make_unique<values::Vector>(
                                function_state.make_wrapper(
                                    std::make_unique<values::Value>(
                                        values::Value_type::Value)))),
                        std::move(defining_tokens));

                    break;
                }

                auto const& begin_pack = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const& end_pack = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());

                if (begin_pack.iota) {
                    throw viua::tooling::errors::compile_time::Error_wrapper{}
                        .append(viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::
                                Compile_time_error::Empty_error,
                            instruction.operands.at(1)->tokens().at(1),
                            "begin-packing register index must not be iota"});
                }
                if (end_pack.iota) {
                    throw viua::tooling::errors::compile_time::Error_wrapper{}
                        .append(viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::
                                Compile_time_error::Empty_error,
                            instruction.operands.at(2)->tokens().at(1),
                            "end-packing register index must not be iota"});
                }

                if (begin_pack.register_set != end_pack.register_set) {
                    throw viua::tooling::errors::compile_time::Error_wrapper{}
                        .append(
                            viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::Empty_error,
                                instruction.operands.at(1)->tokens().at(2),
                                "begin-packing and end-packing registers must "
                                "be in the same set"}
                                .add(instruction.operands.at(2)->tokens().at(
                                    2)));
                }

                auto const& first_packed =
                    function_state.resolve_index(begin_pack);
                auto const& last_packed =
                    function_state.resolve_index(end_pack);

                if (first_packed > last_packed) {
                    throw viua::tooling::errors::compile_time::Error_wrapper{}
                        .append(viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::
                                Compile_time_error::Empty_error,
                            instruction.operands.at(1)->tokens().at(1),
                            "begin-packing register index must be less than or "
                            "equal to the end-packing register index"}
                                    .add(
                                        instruction.operands.at(2)->tokens().at(
                                            1)));
                }

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const first_type =
                    function_state
                        .type_of(first_packed, begin_pack.register_set)
                        .to_simple();
                for (auto check_pack = first_packed; check_pack <= last_packed;
                     ++check_pack) {
                    if (not function_state.defined(check_pack,
                                                   begin_pack.register_set)) {
                        throw viua::tooling::errors::compile_time::
                            Error_wrapper{}
                                .append(
                                    viua::tooling::errors::compile_time::Error{
                                        viua::tooling::errors::compile_time::
                                            Compile_time_error::Empty_error,
                                        instruction.token(0),
                                        ("packing empty "
                                         + to_string(begin_pack.register_set)
                                         + " register "
                                         + std::to_string(check_pack))}
                                        .add(instruction.operands.at(1)
                                                 ->tokens()
                                                 .at(1))
                                        .add(instruction.operands.at(1)
                                                 ->tokens()
                                                 .at(2))
                                        .add(instruction.operands.at(2)
                                                 ->tokens()
                                                 .at(1))
                                        .add(instruction.operands.at(2)
                                                 ->tokens()
                                                 .at(2)));
                    }

                    if (not function_state.assume_type(
                            check_pack, begin_pack.register_set, first_type)) {
                        auto error =
                            viua::tooling::errors::compile_time::Error_wrapper{}
                                .append(
                                    viua::tooling::errors::compile_time::Error{
                                        viua::tooling::errors::compile_time::
                                            Compile_time_error::Type_mismatch,
                                        instruction.token(0),
                                        "values packed into a vector must be "
                                        "of the same type"});

                        auto const& definition_location =
                            function_state.defined_at(check_pack,
                                                      begin_pack.register_set);
                        error.append(viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::
                                Compile_time_error::Empty_error,
                            definition_location.at(0),
                            ("expected `" + to_string(first_type) + "' got `"
                             + to_string(function_state
                                             .type_of(check_pack,
                                                      begin_pack.register_set)
                                             .to_simple())
                             + "'")}
                                         .note("defined here"));

                        error.append(
                            viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::Empty_error,
                                function_state
                                    .defined_at(first_packed,
                                                begin_pack.register_set)
                                    .at(0)}
                                .note("first packed value was defined here"));

                        throw error;
                    }
                }

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Vector>(function_state.type_of(
                            first_packed, begin_pack.register_set))),
                    std::move(defining_tokens));

                for (auto check_pack = first_packed; check_pack <= last_packed;
                     ++check_pack) {
                    function_state.erase_register(
                        check_pack,
                        begin_pack.register_set,
                        std::vector<viua::tooling::libs::lexer::Token>{
                            line->token(0)});
                }

                break;
            }
            case VINSERT:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access,
                    function_state.type_of(source_index, source.register_set)
                        .to_simple());
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());
                auto const dest_index = throw_if_empty(function_state, dest);
                auto const dest_type_signature = maybe_with_pointer(
                    dest.access,
                    prepend(values::Value_type::Vector,
                            function_state
                                .type_of(source_index, source.register_set)
                                .to_simple()));
                throw_if_invalid_type(
                    function_state, dest, dest_index, dest_type_signature);

                auto const& index = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());
                auto const index_index = throw_if_empty(function_state, index);
                auto const index_type_signature =
                    maybe_with_pointer(index.access,
                                       std::vector<values::Value_type>{
                                           values::Value_type::Integer});
                throw_if_invalid_type(
                    function_state, index, index_index, index_type_signature);

                {
                    auto& wrapper =
                        (dest_type_signature.front()
                         == values::Value_type::Pointer)
                            ? static_cast<values::Vector&>(
                                static_cast<values::Pointer&>(
                                    function_state
                                        .type_of(dest_index, dest.register_set)
                                        .value())
                                    .of()
                                    .value())
                            : static_cast<values::Vector&>(
                                function_state
                                    .type_of(dest_index, dest.register_set)
                                    .value());
                    if (source.access
                        == viua::internals::Access_specifier::
                            POINTER_DEREFERENCE) {
                        wrapper.of(
                            static_cast<values::Pointer&>(
                                function_state
                                    .type_of(source_index, source.register_set)
                                    .value())
                                .of());
                    } else {
                        wrapper.of(function_state.type_of(source_index,
                                                          source.register_set));
                    }
                }

                if (source.access
                    != viua::internals::Access_specifier::POINTER_DEREFERENCE) {
                    function_state.erase_register(
                        source_index,
                        source.register_set,
                        std::vector<viua::tooling::libs::lexer::Token>{
                            line->token(0)});
                }

                break;
            }
            case VPUSH:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access,
                    function_state.type_of(source_index, source.register_set)
                        .to_simple());
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());
                auto const dest_index = throw_if_empty(function_state, dest);
                auto const dest_type_signature = maybe_with_pointer(
                    dest.access,
                    prepend(values::Value_type::Vector,
                            function_state
                                .type_of(source_index, source.register_set)
                                .to_simple()));
                throw_if_invalid_type(
                    function_state, dest, dest_index, dest_type_signature);

                {
                    auto& wrapper =
                        (dest_type_signature.front()
                         == values::Value_type::Pointer)
                            ? static_cast<values::Vector&>(
                                static_cast<values::Pointer&>(
                                    function_state
                                        .type_of(dest_index, dest.register_set)
                                        .value())
                                    .of()
                                    .value())
                            : static_cast<values::Vector&>(
                                function_state
                                    .type_of(dest_index, dest.register_set)
                                    .value());
                    if (source.access
                        == viua::internals::Access_specifier::
                            POINTER_DEREFERENCE) {
                        wrapper.of(
                            static_cast<values::Pointer&>(
                                function_state
                                    .type_of(source_index, source.register_set)
                                    .value())
                                .of());
                    } else {
                        wrapper.of(function_state.type_of(source_index,
                                                          source.register_set));
                    }
                }

                if (source.access
                    != viua::internals::Access_specifier::POINTER_DEREFERENCE) {
                    function_state.erase_register(
                        source_index,
                        source.register_set,
                        std::vector<viua::tooling::libs::lexer::Token>{
                            line->token(0)});
                }

                break;
            }
            case VPOP:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access,
                    {values::Value_type::Vector, values::Value_type::Value});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                if (instruction.operands.at(2)->type()
                    == parser::Operand_type::Register_address) {
                    auto const& index = *static_cast<Register_address const*>(
                        instruction.operands.at(2).get());
                    auto const index_index =
                        throw_if_empty(function_state, index);
                    auto const index_type_signature = maybe_with_pointer(
                        index.access, {values::Value_type::Integer});
                    throw_if_invalid_type(function_state,
                                          index,
                                          index_index,
                                          index_type_signature);
                } else if (instruction.operands.at(2)->type()
                           == parser::Operand_type::Void) {
                    // do nothing
                } else {
                    // do nothing, errors should be handled in earlier stages
                }

                if (instruction.operands.at(0)->type()
                    == parser::Operand_type::Register_address) {
                    auto const& dest = *static_cast<Register_address const*>(
                        instruction.operands.at(0).get());

                    auto defining_tokens =
                        std::vector<viua::tooling::libs::lexer::Token>{};
                    defining_tokens.push_back(line->token(0));
                    copy_whole(dest.tokens(),
                               std::back_inserter(defining_tokens));

                    auto const dest_index = function_state.resolve_index(dest);
                    function_state.define_register(
                        dest_index,
                        dest.register_set,
                        static_cast<values::Vector const&>(
                            function_state
                                .type_of(source_index, source.register_set)
                                .value())
                            .of(),
                        std::move(defining_tokens));
                } else if (instruction.operands.at(0)->type()
                           == parser::Operand_type::Void) {
                    // do nothing
                } else {
                    // do nothing, errors should be handled in earlier stages
                }

                break;
            }
            case VAT:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const& index = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access,
                    {values::Value_type::Vector, values::Value_type::Value});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const index_index = throw_if_empty(function_state, index);
                auto const index_type_signature =
                    std::vector<values::Value_type>{
                        values::Value_type::Integer};
                throw_if_invalid_type(
                    function_state, index, index_index, index_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());
                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                if (source.access
                    == viua::internals::Access_specifier::POINTER_DEREFERENCE) {
                    function_state.define_register(
                        dest_index,
                        dest.register_set,
                        function_state.make_wrapper(
                            std::make_unique<values::Pointer>(
                                static_cast<values::Vector const&>(
                                    static_cast<values::Pointer const&>(
                                        function_state
                                            .type_of(source_index,
                                                     source.register_set)
                                            .value())
                                        .of()
                                        .value())
                                    .of())),
                        std::move(defining_tokens));
                } else {
                    function_state.define_register(
                        dest_index,
                        dest.register_set,
                        function_state.make_wrapper(
                            std::make_unique<values::Pointer>(
                                static_cast<values::Vector const&>(
                                    function_state
                                        .type_of(source_index,
                                                 source.register_set)
                                        .value())
                                    .of())),
                        std::move(defining_tokens));
                }

                break;
            }
            case VLEN:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access,
                    {values::Value_type::Vector, values::Value_type::Value});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Integer>()),
                    std::move(defining_tokens));

                break;
            }
            case BOOL:
            {
                // not implemented
                break;
            }
            case NOT:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                throw_if_empty(function_state, source);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Boolean>()),
                    std::move(defining_tokens));

                break;
            }
            case AND:
            case OR:
            {
                auto const& lhs = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                throw_if_empty(function_state, lhs);

                auto const& rhs = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());
                throw_if_empty(function_state, rhs);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Boolean>()),
                    std::move(defining_tokens));

                break;
            }
            case BITS:
            {
                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                using viua::tooling::libs::parser::Bits_literal;
                function_state.define_register(
                    function_state.resolve_index(dest),
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Bits>()),
                    std::move(defining_tokens));

                break;
            }
            case BITNOT:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Bits});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Bits>()),
                    std::move(defining_tokens));

                break;
            }
            case BITAND:
            case BITOR:
            case BITXOR:
            {
                auto const& lhs = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const lhs_index = throw_if_empty(function_state, lhs);
                auto const lhs_type_signature =
                    maybe_with_pointer(lhs.access, {values::Value_type::Bits});
                throw_if_invalid_type(
                    function_state, lhs, lhs_index, lhs_type_signature);

                auto const& rhs = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const rhs_index = throw_if_empty(function_state, rhs);
                auto const rhs_type_signature =
                    maybe_with_pointer(rhs.access, {values::Value_type::Bits});
                throw_if_invalid_type(
                    function_state, rhs, rhs_index, rhs_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Bits>()),
                    std::move(defining_tokens));

                break;
            }
            case BITSWIDTH:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Bits});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Integer>()),
                    std::move(defining_tokens));

                break;
            }
            case BITAT:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const& index = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Bits});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const index_index = throw_if_empty(function_state, index);
                auto const index_type_signature =
                    std::vector<values::Value_type>{
                        values::Value_type::Integer};
                throw_if_invalid_type(
                    function_state, index, index_index, index_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());
                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Boolean>()),
                    std::move(defining_tokens));

                break;
            }
            case BITSET:
            {
                auto const& target = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());
                auto const target_index =
                    throw_if_empty(function_state, target);
                auto const target_type_signature = maybe_with_pointer(
                    target.access, {values::Value_type::Bits});
                throw_if_invalid_type(function_state,
                                      target,
                                      target_index,
                                      target_type_signature);

                auto const& index = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());

                auto const index_index = throw_if_empty(function_state, index);
                auto const index_type_signature = maybe_with_pointer(
                    index.access, {values::Value_type::Integer});
                throw_if_invalid_type(
                    function_state, index, index_index, index_type_signature);

                /*
                 * Type checking for the third operand is not needed since it
                 * was performed by the earlier stages.
                 */

                break;
            }
            case SHL:
            case SHR:
            case ASHL:
            case ASHR:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Bits});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const& offset = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());
                auto const offset_index =
                    throw_if_empty(function_state, offset);
                auto const offset_type_signature = maybe_with_pointer(
                    offset.access, {values::Value_type::Integer});
                throw_if_invalid_type(function_state,
                                      offset,
                                      offset_index,
                                      offset_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());
                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Bits>()),
                    std::move(defining_tokens));

                break;
            }
            case ROL:
            case ROR:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Bits});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const& offset = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const offset_index =
                    throw_if_empty(function_state, offset);
                auto const offset_type_signature = maybe_with_pointer(
                    offset.access, {values::Value_type::Integer});
                throw_if_invalid_type(function_state,
                                      offset,
                                      offset_index,
                                      offset_type_signature);

                break;
            }
            case BITSEQ:
            case BITSLT:
            case BITSLTE:
            case BITSGT:
            case BITSGTE:
            case BITAEQ:
            case BITALT:
            case BITALTE:
            case BITAGT:
            case BITAGTE:
            {
                auto const& lhs = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const& rhs = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());

                auto const lhs_index = throw_if_empty(function_state, lhs);
                auto const lhs_type_signature =
                    maybe_with_pointer(lhs.access, {values::Value_type::Bits});
                throw_if_invalid_type(
                    function_state, lhs, lhs_index, lhs_type_signature);

                auto const rhs_index = throw_if_empty(function_state, rhs);
                auto const rhs_type_signature =
                    maybe_with_pointer(rhs.access, {values::Value_type::Bits});
                throw_if_invalid_type(
                    function_state, rhs, rhs_index, rhs_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());
                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Boolean>()),
                    std::move(defining_tokens));

                break;
            }
            case WRAPINCREMENT:
            case WRAPDECREMENT:
            case CHECKEDSINCREMENT:
            case CHECKEDSDECREMENT:
            case CHECKEDUINCREMENT:
            case CHECKEDUDECREMENT:
            case SATURATINGSINCREMENT:
            case SATURATINGSDECREMENT:
            case SATURATINGUINCREMENT:
            case SATURATINGUDECREMENT:
            {
                auto const& target = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto const target_index =
                    throw_if_empty(function_state, target);
                auto const target_type_signature = maybe_with_pointer(
                    target.access, {values::Value_type::Bits});
                throw_if_invalid_type(function_state,
                                      target,
                                      target_index,
                                      target_type_signature);

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(target.tokens(),
                           std::back_inserter(defining_tokens));
                function_state.mutate_register(target_index,
                                               target.register_set,
                                               std::move(defining_tokens));

                break;
            }
            case WRAPADD:
            case WRAPSUB:
            case WRAPMUL:
            case WRAPDIV:
            case CHECKEDSADD:
            case CHECKEDSSUB:
            case CHECKEDSMUL:
            case CHECKEDSDIV:
            case CHECKEDUADD:
            case CHECKEDUSUB:
            case CHECKEDUMUL:
            case CHECKEDUDIV:
            case SATURATINGSADD:
            case SATURATINGSSUB:
            case SATURATINGSMUL:
            case SATURATINGSDIV:
            case SATURATINGUADD:
            case SATURATINGUSUB:
            case SATURATINGUMUL:
            case SATURATINGUDIV:
            {
                auto const& lhs = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const& rhs = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());

                auto const lhs_index = throw_if_empty(function_state, lhs);
                auto const lhs_type_signature =
                    maybe_with_pointer(lhs.access,
                                       {
                                           values::Value_type::Bits,
                                       });
                throw_if_invalid_type(
                    function_state, lhs, lhs_index, lhs_type_signature);

                auto const rhs_index = throw_if_empty(function_state, rhs);
                auto const rhs_type_signature =
                    maybe_with_pointer(rhs.access,
                                       {
                                           values::Value_type::Bits,
                                       });
                throw_if_invalid_type(
                    function_state, rhs, rhs_index, rhs_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());
                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Bits>()),
                    std::move(defining_tokens));

                break;
            }
            case COPY:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Value});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const strip_pointer =
                    [](values::Value_wrapper wrapper) -> values::Value_wrapper {
                    if (wrapper.value().type() == values::Value_type::Pointer) {
                        return static_cast<values::Pointer const&>(
                                   wrapper.value())
                            .of();
                    } else {
                        return wrapper;
                    }
                };

                auto const dest_index = function_state.resolve_index(dest);

                if (dest.register_set
                    == viua::bytecode::codec::Register_set::Arguments) {
                    if (not spawned_frame) {
                        auto error =
                            viua::tooling::errors::compile_time::Error_wrapper{}
                                .append(
                                    viua::tooling::errors::compile_time::Error{
                                        Compile_time_error::
                                            Argument_pass_without_a_frame,
                                        instruction.token(0)}
                                        .add(dest.tokens().at(2))
                                        .comment(
                                            "`arguments' register set is only "
                                            "valid when there "
                                            "is an unused frame available"));
                        throw error;
                    }
                    if (spawned_frame->filled_parameters.count(dest.index)) {
                        auto error =
                            viua::tooling::errors::compile_time::Error_wrapper{}
                                .append(
                                    viua::tooling::errors::compile_time::Error{
                                        Compile_time_error::
                                            Argument_pass_overwrites,
                                        instruction.token(0)}
                                        .add(dest.tokens().at(2)))
                                .append(
                                    viua::tooling::errors::compile_time::Error{
                                        Compile_time_error::Empty_error,
                                        spawned_frame->filled_parameters.at(
                                            dest.index)}
                                        .note(
                                            "argument previously passed here"));
                        throw error;
                    }
                    spawned_frame->filled_parameters.emplace(
                        dest.index, instruction.token(0));
                } else {
                    function_state.define_register(
                        dest_index,
                        dest.register_set,
                        strip_pointer(function_state.type_of(
                            source_index, source.register_set)),
                        std::move(defining_tokens));
                }

                break;
            }
            case PTR:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Value});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Pointer>(
                            function_state.type_of(source_index,
                                                   source.register_set))),
                    std::move(defining_tokens));

                break;
            }
            case PTRLIVE:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access,
                    {values::Value_type::Pointer, values::Value_type::Value});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Boolean>()),
                    std::move(defining_tokens));

                break;
            }
            case SWAP:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());
                auto const source_index =
                    throw_if_empty(function_state, source);
                if (source.access
                    == viua::internals::Access_specifier::POINTER_DEREFERENCE) {
                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::Empty_error,
                                source.tokens().at(0),
                                "only direct access allowed for `swap' "
                                "instruction"});
                    throw error;
                }

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const dest_index = throw_if_empty(function_state, dest);
                if (dest.access
                    == viua::internals::Access_specifier::POINTER_DEREFERENCE) {
                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::Empty_error,
                                dest.tokens().at(0),
                                "only direct access allowed for `swap' "
                                "instruction"});
                    throw error;
                }

                auto source_type =
                    function_state.type_of(source_index, source.register_set);
                auto dest_type =
                    function_state.type_of(dest_index, dest.register_set);

                function_state.erase_register(
                    source_index,
                    source.register_set,
                    [&instruction, &source]()
                        -> std::vector<viua::tooling::libs::lexer::Token> {
                        auto tokens =
                            std::vector<viua::tooling::libs::lexer::Token>{};
                        tokens.push_back(instruction.tokens().at(0));
                        copy_whole(source.tokens(), std::back_inserter(tokens));
                        return tokens;
                    }());
                function_state.erase_register(
                    dest_index,
                    dest.register_set,
                    [&instruction, &dest]()
                        -> std::vector<viua::tooling::libs::lexer::Token> {
                        auto tokens =
                            std::vector<viua::tooling::libs::lexer::Token>{};
                        tokens.push_back(instruction.tokens().at(0));
                        copy_whole(dest.tokens(), std::back_inserter(tokens));
                        return tokens;
                    }());

                function_state.define_register(
                    source_index,
                    source.register_set,
                    dest_type,
                    [&instruction, &source]()
                        -> std::vector<viua::tooling::libs::lexer::Token> {
                        auto tokens =
                            std::vector<viua::tooling::libs::lexer::Token>{};
                        tokens.push_back(instruction.tokens().at(0));
                        copy_whole(source.tokens(), std::back_inserter(tokens));
                        return tokens;
                    }());
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    source_type,
                    [&instruction, &dest]()
                        -> std::vector<viua::tooling::libs::lexer::Token> {
                        auto tokens =
                            std::vector<viua::tooling::libs::lexer::Token>{};
                        tokens.push_back(instruction.tokens().at(0));
                        copy_whole(dest.tokens(), std::back_inserter(tokens));
                        return tokens;
                    }());

                break;
            }
            case DELETE:
            {
                auto const& target = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto const target_index =
                    throw_if_empty(function_state, target);
                if (target.access
                    == viua::internals::Access_specifier::POINTER_DEREFERENCE) {
                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::
                                        Invalid_access_type_specifier,
                                target.tokens().at(0),
                                "only direct access allowed for `delete' "
                                "instruction"});
                    throw error;
                }

                auto erasing_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                erasing_tokens.push_back(instruction.tokens().at(0));
                copy_whole(target.tokens(), std::back_inserter(erasing_tokens));

                function_state.erase_register(
                    target_index, target.register_set, erasing_tokens);

                break;
            }
            case ISNULL:
            {
                // FIXME do nothing, but later add a warning that such-and-such
                // instruction is not covered by static analyser
                break;
            }
            case PRINT:
            case ECHO:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Value});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                break;
            }
            case CAPTURE:
            case CAPTURECOPY:
            case CAPTUREMOVE:
            {
                // FIXME TODO
                break;
            }
            case CLOSURE:
            {
                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto const closure_name =
                    (instruction.operands.at(1)->tokens().at(0).str() + '/'
                     + instruction.operands.at(1)->tokens().at(2).str());
                if (fragments.closure_fragments.count(closure_name) == 0) {
                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(
                                viua::tooling::errors::compile_time::Error{
                                    viua::tooling::errors::compile_time::
                                        Compile_time_error::Empty_error,
                                    instruction.operands.at(1)->tokens().at(0),
                                    "no closure named `" + closure_name + "'"}
                                    .add(
                                        instruction.operands.at(1)->tokens().at(
                                            1))
                                    .add(
                                        instruction.operands.at(1)->tokens().at(
                                            2)));
                    throw error;
                }

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                function_state.define_register(
                    function_state.resolve_index(dest),
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Closure>(closure_name)),
                    std::move(defining_tokens));

                break;
            }
            case FUNCTION:
            {
                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto const function_name =
                    (instruction.operands.at(1)->tokens().at(0).str() + '/'
                     + instruction.operands.at(1)->tokens().at(2).str());

                if (fragments.function_fragments.count(function_name) == 0) {
                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(
                                viua::tooling::errors::compile_time::Error{
                                    viua::tooling::errors::compile_time::
                                        Compile_time_error::Empty_error,
                                    instruction.operands.at(1)->tokens().at(0),
                                    "no function named `" + function_name + "'"}
                                    .add(
                                        instruction.operands.at(1)->tokens().at(
                                            1))
                                    .add(
                                        instruction.operands.at(1)->tokens().at(
                                            2)));
                    throw error;
                }

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                function_state.define_register(
                    function_state.resolve_index(dest),
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Function>(function_name)),
                    std::move(defining_tokens));

                break;
            }
            case FRAME:
            {
                if (spawned_frame) {
                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::
                                        Overwrite_of_unused_frame,
                                instruction.token(0)})
                            .append(viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::Empty_error,
                                spawned_frame_where}
                                        .note("unused frame spawned here"));
                    throw error;
                }

                auto const no_of_allocated_params =
                    static_cast<Register_address const*>(
                        instruction.operands.at(0).get())
                        ->index;
                spawned_frame = std::make_unique<Frame_representation>(
                    no_of_allocated_params);
                spawned_frame_where = instruction.token(0);

                break;
            }
            case ALLOCATE_REGISTERS:
            {
                /*
                 * Register allocation is handled before the analyser begins
                 * its work so it is not checked here.
                 * However, if this instruction is found inside the function
                 * it is an error since registers cannot be reallocted while
                 * the function is running.
                 */
                throw viua::tooling::errors::compile_time::Error_wrapper{}
                    .append(viua::tooling::errors::compile_time::Error{
                        Compile_time_error::Empty_error,
                        line->token(0),
                        "cannot reallocate register set"});
            }
            case CALL:
            {
            }
            case TAILCALL:
            {
            }
            case DEFER:
            {
            }
            case PROCESS:
            {
                if (not spawned_frame) {
                    throw viua::tooling::errors::compile_time::Error_wrapper{}
                        .append(viua::tooling::errors::compile_time::Error{
                            Compile_time_error::Call_without_a_frame,
                            instruction.tokens().at(0)});
                }

                for (auto const each :
                     int_range(spawned_frame->allocated_parameters)) {
                    if (not spawned_frame->filled_parameters.count(each)) {
                        auto error =
                            viua::tooling::errors::compile_time::Error_wrapper{}
                                .append(
                                    viua::tooling::errors::compile_time::Error{
                                        Compile_time_error::
                                            Call_with_empty_slot,
                                        instruction.token(0),
                                        "slot " + std::to_string(each)})
                                .append(
                                    viua::tooling::errors::compile_time::Error{
                                        Compile_time_error::Empty_error,
                                        spawned_frame_where}
                                        .note("frame spawned here"));
                        throw error;
                    }
                }
                auto const called_function_name =
                    [&instruction]() -> std::string {
                    if (instruction.opcode == CALL
                        or instruction.opcode == PROCESS) {
                        auto const& oper = instruction.operands.at(1);
                        return oper->tokens().at(0).str()
                               + oper->tokens().at(1).str()
                               + oper->tokens().at(2).str();
                    }
                    return "";
                }();
                std::cerr << "  calling: " << called_function_name << " from "
                          << analysed_region_name << '\n';
                // FIXME we have to check "history" of block enters because a
                // function may call itself from inside a block
                if ((analysed_region_name == called_function_name)
                    and not after_conditional_branch) {
                    throw viua::tooling::errors::compile_time::Error_wrapper{}
                        .append(viua::tooling::errors::compile_time::Error{
                            Compile_time_error::Empty_error  // FIXME add custom
                                                             // error type
                            ,
                            line->token(0),
                            "endless recursion detected"}
                                    .note("this function calls itself on a "
                                          "path with no conditional branches"));
                }

                /*
                 * Calling a function (it does not matter in what way) consumes
                 * the frame.
                 */
                spawned_frame.reset(nullptr);

                if (instruction.operands.at(0).get()->type()
                        == Operand_type::Register_address
                    and (instruction.opcode == CALL
                         or instruction.opcode == PROCESS)) {
                    auto const& dest = *static_cast<Register_address const*>(
                        instruction.operands.at(0).get());

                    auto defining_tokens =
                        std::vector<viua::tooling::libs::lexer::Token>{};
                    defining_tokens.push_back(line->token(0));
                    copy_whole(dest.tokens(),
                               std::back_inserter(defining_tokens));

                    function_state.define_register(
                        function_state.resolve_index(dest),
                        dest.register_set,
                        function_state.make_wrapper(
                            std::make_unique<values::Value>(
                                values::Value_type::Value)),
                        std::move(defining_tokens));
                }

                break;
            }
            case SELF:
            {
                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                function_state.define_register(
                    function_state.resolve_index(dest),
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Pid>()),
                    std::move(defining_tokens));

                break;
            }
            case PIDEQ:
            {
                auto const& lhs = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const& rhs = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());

                auto const lhs_index = throw_if_empty(function_state, lhs);
                auto const lhs_type_signature =
                    maybe_with_pointer(lhs.access, {values::Value_type::Pid});
                throw_if_invalid_type(
                    function_state, lhs, lhs_index, lhs_type_signature);

                auto const rhs_index = throw_if_empty(function_state, rhs);
                auto const rhs_type_signature =
                    maybe_with_pointer(rhs.access, {values::Value_type::Pid});
                throw_if_invalid_type(
                    function_state, rhs, rhs_index, rhs_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());
                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Boolean>()),
                    std::move(defining_tokens));

                break;
            }
            case JOIN:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());

                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Pid});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(std::make_unique<values::Value>(
                        values::Value_type::Value)),
                    std::move(defining_tokens));

                break;
            }
            case SEND:
            {
                auto const& target = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto const target_index =
                    throw_if_empty(function_state, target);
                auto const target_type_signature = maybe_with_pointer(
                    target.access, {values::Value_type::Pid});
                throw_if_invalid_type(function_state,
                                      target,
                                      target_index,
                                      target_type_signature);

                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const source_index =
                    throw_if_empty(function_state, source);

                if (source.access
                    != viua::internals::Access_specifier::POINTER_DEREFERENCE) {
                    function_state.erase_register(
                        source_index,
                        source.register_set,
                        [&instruction, &source]()
                            -> std::vector<viua::tooling::libs::lexer::Token> {
                            auto tokens = std::vector<
                                viua::tooling::libs::lexer::Token>{};
                            tokens.push_back(instruction.tokens().at(0));
                            copy_whole(source.tokens(),
                                       std::back_inserter(tokens));
                            return tokens;
                        }());
                }

                break;
            }
            case RECEIVE:
            {
                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(std::make_unique<values::Value>(
                        values::Value_type::Value)),
                    std::move(defining_tokens));

                break;
            }
            case WATCHDOG:
            {
                auto const called_function_name =
                    instruction.operands.at(0)->tokens().at(0).str()
                    + instruction.operands.at(0)->tokens().at(1).str()
                    + instruction.operands.at(0)->tokens().at(2).str();
                std::cerr << "  setting: " << called_function_name
                          << " as watchdog from " << analysed_region_name
                          << '\n';

                if (not spawned_frame) {
                    throw viua::tooling::errors::compile_time::Error_wrapper{}
                        .append(viua::tooling::errors::compile_time::Error{
                            Compile_time_error::Call_without_a_frame,
                            instruction.operands.at(0)->tokens().at(0),
                            called_function_name});
                }

                for (auto const each :
                     int_range(spawned_frame->allocated_parameters)) {
                    if (not spawned_frame->filled_parameters.count(each)) {
                        auto error =
                            viua::tooling::errors::compile_time::Error_wrapper{}
                                .append(
                                    viua::tooling::errors::compile_time::Error{
                                        Compile_time_error::
                                            Call_with_empty_slot,
                                        instruction.token(0),
                                        "slot " + std::to_string(each)})
                                .append(
                                    viua::tooling::errors::compile_time::Error{
                                        Compile_time_error::Empty_error,
                                        spawned_frame_where}
                                        .note("frame spawned here"));
                        throw error;
                    }
                }

                /*
                 * Calling a function (it does not matter in what way) consumes
                 * the frame.
                 */
                spawned_frame.reset(nullptr);

                break;
            }
            case JUMP:
            {
                auto target = decltype(i){0};

                if (instruction.operands.at(0)->type()
                    == Operand_type::Jump_offset) {
                    using viua::tooling::libs::parser::Jump_offset;

                    auto const& jump_offset = *static_cast<Jump_offset const*>(
                        instruction.operands.at(0).get());
                    target =
                        ((jump_offset.value.at(0) == '+')
                             ? (i + std::stoul(jump_offset.value.substr(1)))
                             : (i - std::stoul(jump_offset.value.substr(1))));
                    std::cerr << "  jumping to (offset): " << jump_offset.value
                              << " (instruction " << target << ')' << '\n';
                } else if (instruction.operands.at(0)->type()
                           == Operand_type::Jump_label) {
                    using viua::tooling::libs::parser::Jump_label;

                    auto const& jump_label = *static_cast<Jump_label const*>(
                        instruction.operands.at(0).get());
                    target = label_map.at(jump_label.value).source_line;
                    std::cerr << "  jumping to (label): " << jump_label.value
                              << " (instruction "
                              << label_map.at(jump_label.value).instruction
                              << ')' << '\n';
                }

                try {
                    analyse_single_arm(fn,
                                       bl,
                                       fragments,
                                       analyser_state,
                                       function_state,
                                       after_conditional_branch,
                                       annotated_body,
                                       label_map,
                                       target);
                } catch (
                    viua::tooling::errors::compile_time::Error_wrapper& e) {
                    e.append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Empty_error,
                        instruction.tokens().at(0),
                        "after taking a branch"});
                    throw;
                }

                /*
                 * Return instead of break because all the analysis
                 * that would normally be performed at exit (after
                 * the final function state and all of its side
                 * effects (e.g. closure instantiations, function
                 * calls) are known) will be performed by the nested
                 * call to analyse_single_arm().
                 *
                 * JUMP affects the control flow directrly, and this
                 * is reflected in the way the analyser operates.
                 */
                return arm_result;
            }
            case IF:
            {
                auto true_arm_result = Arm_result{};

                try {
                    auto target = decltype(i){0};

                    if (instruction.operands.at(1)->type()
                        == Operand_type::Jump_offset) {
                        using viua::tooling::libs::parser::Jump_offset;

                        auto const& jump_offset =
                            *static_cast<Jump_offset const*>(
                                instruction.operands.at(1).get());
                        target =
                            ((jump_offset.value.at(0) == '+')
                                 ? (i + std::stoul(jump_offset.value.substr(1)))
                                 : (i
                                    - std::stoul(jump_offset.value.substr(1))));
                        std::cerr
                            << "  jumping to (offset): " << jump_offset.value
                            << " (instruction " << target << ')' << '\n';
                    } else if (instruction.operands.at(1)->type()
                               == Operand_type::Jump_label) {
                        using viua::tooling::libs::parser::Jump_label;

                        auto const& jump_label =
                            *static_cast<Jump_label const*>(
                                instruction.operands.at(1).get());
                        target = label_map.at(jump_label.value).source_line;
                        std::cerr
                            << "  jumping to (label): " << jump_label.value
                            << " (instruction "
                            << label_map.at(jump_label.value).instruction << ')'
                            << '\n';
                    }

                    auto cloned     = function_state.clone();
                    true_arm_result = analyse_single_arm(fn,
                                                         bl,
                                                         fragments,
                                                         analyser_state,
                                                         cloned,
                                                         true,
                                                         annotated_body,
                                                         label_map,
                                                         target);
                } catch (
                    viua::tooling::errors::compile_time::Error_wrapper& e) {
                    e.append(
                        viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::
                                Compile_time_error::Empty_error,
                            instruction.tokens().at(0),
                            "after taking true branch"}
                            .add(instruction.operands.at(1)->tokens().at(0)));
                    throw;
                }

                try {
                    auto target = decltype(i){0};

                    if (instruction.operands.at(2)->type()
                        == Operand_type::Jump_offset) {
                        using viua::tooling::libs::parser::Jump_offset;

                        auto const& jump_offset =
                            *static_cast<Jump_offset const*>(
                                instruction.operands.at(2).get());
                        target =
                            ((jump_offset.value.at(0) == '+')
                                 ? (i + std::stoul(jump_offset.value.substr(1)))
                                 : (i
                                    - std::stoul(jump_offset.value.substr(1))));
                        std::cerr
                            << "  jumping to (offset): " << jump_offset.value
                            << " (instruction " << target << ')' << '\n';
                    } else if (instruction.operands.at(2)->type()
                               == Operand_type::Jump_label) {
                        using viua::tooling::libs::parser::Jump_label;

                        auto const& jump_label =
                            *static_cast<Jump_label const*>(
                                instruction.operands.at(2).get());
                        target = label_map.at(jump_label.value).source_line;
                        std::cerr
                            << "  jumping to (label): " << jump_label.value
                            << " (instruction "
                            << label_map.at(jump_label.value).instruction << ')'
                            << '\n';
                    }

                    auto cloned = function_state.clone();
                    analyse_single_arm(
                        fn,
                        bl,
                        fragments,
                        analyser_state,
                        cloned,
                        /*
                         * False arm is only conditionally executed if the true
                         * arm did not enter the same path as it. Otherwise, we
                         * assume that it is executed unconditionally because it
                         * was executed by both branches of the if instruction.
                         */
                        (true_arm_result.analysed_lines.count(target) == 0),
                        annotated_body,
                        label_map,
                        target);
                } catch (
                    viua::tooling::errors::compile_time::Error_wrapper& e) {
                    e.append(
                        viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::
                                Compile_time_error::Empty_error,
                            instruction.tokens().at(0),
                            "after taking false branch"}
                            .add(instruction.operands.at(2)->tokens().at(0)));
                    throw;
                }

                return arm_result;
            }
            case THROW:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                throw_if_empty(function_state, source);
                if (source.access
                    == viua::internals::Access_specifier::POINTER_DEREFERENCE) {
                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::
                                        Invalid_access_type_specifier,
                                source.tokens().at(0),
                                "only direct access allowed for `throw' "
                                "instruction"});
                    throw error;
                }

                /*
                 * Return from the function as throwing aborts the normal flow.
                 */
                return arm_result;
            }
            case CATCH:
            {
                if (not spawned_catch_frame) {
                    auto const name_of_the_block =
                        instruction.operands.at(1)->tokens().at(0);
                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::
                                        Catch_without_a_catch_frame,
                                instruction.token(0)});
                    throw error;
                }
                break;
            }
            case DRAW:
            {
                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                function_state.define_register(
                    function_state.resolve_index(dest),
                    dest.register_set,
                    function_state.make_wrapper(
                        // FIXME maybe use an exception type after exceptions
                        // are fully specified
                        std::make_unique<values::Value>(
                            values::Value_type::Value)),
                    std::move(defining_tokens));

                break;
            }
            case TRY:
            {
                if (spawned_catch_frame) {
                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::
                                        Overwrite_of_unused_frame,
                                instruction.token(0)})
                            .append(
                                viua::tooling::errors::compile_time::Error{
                                    viua::tooling::errors::compile_time::
                                        Compile_time_error::Empty_error,
                                    spawned_frame_where}
                                    .note("unused catch frame spawned here"));
                    throw error;
                }

                spawned_catch_frame       = true;
                spawned_catch_frame_where = instruction.token(0);

                break;
            }
            case ENTER:
            {
                spawned_catch_frame = false;

                auto const& block_name_token =
                    instruction.operands.at(0)->tokens().at(0);
                auto const block_name = block_name_token.str();

                if (not fragments.block_fragments.count(block_name)) {
                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::
                                        Reference_to_undefined_block,
                                block_name_token,
                                block_name});
                    throw error;
                }

                try {
                    auto const& block =
                        fragments.block_fragments.at(block_name);
                    auto const block_body           = block.body();
                    auto const block_annotated_body = annotate_body(block_body);
                    auto const block_label_map =
                        create_label_map(block_body, block_annotated_body);

                    std::cerr << "entering block: " << block_name << '\n';

                    analyse_single_arm(fn,
                                       block,
                                       fragments,
                                       analyser_state,
                                       function_state,
                                       false,
                                       block_annotated_body,
                                       block_label_map,
                                       0);

                    std::cerr << "left block:     " << block_name << '\n';
                } catch (
                    viua::tooling::errors::compile_time::Error_wrapper& e) {
                    e.append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Empty_error,
                        instruction.tokens().at(0),
                        "after entering block " + block_name}
                                 .add(block_name_token));
                    throw;
                }
                break;
            }
            case LEAVE:
            {
                return arm_result;
            }
            case IMPORT:
            {
                /*
                 * This instruction does not touch any registers.
                 */
                break;
            }
            case ATOM:
            {
                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                function_state.define_register(
                    function_state.resolve_index(dest),
                    dest.register_set,
                    function_state.make_wrapper(std::make_unique<values::Atom>(
                        instruction.operands.at(1)->tokens().at(0).str())),
                    std::move(defining_tokens));

                break;
            }
            case ATOMEQ:
            {
                auto const& lhs = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const& rhs = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());

                auto const lhs_index = throw_if_empty(function_state, lhs);
                auto const lhs_type_signature =
                    maybe_with_pointer(lhs.access, {values::Value_type::Atom});
                throw_if_invalid_type(
                    function_state, lhs, lhs_index, lhs_type_signature);

                auto const rhs_index = throw_if_empty(function_state, rhs);
                auto const rhs_type_signature =
                    maybe_with_pointer(rhs.access, {values::Value_type::Atom});
                throw_if_invalid_type(
                    function_state, rhs, rhs_index, rhs_type_signature);

                using values::Atom;
                auto const& lhs_operand = static_cast<Atom const&>(
                    function_state.type_of(lhs_index, lhs.register_set)
                        .value());
                auto const& rhs_operand = static_cast<Atom const&>(
                    function_state.type_of(rhs_index, rhs.register_set)
                        .value());
                if (lhs_operand.known() and rhs_operand.known()) {
                    auto msg = std::ostringstream{};
                    msg << "left-hand side will ";
                    auto const l = lhs_operand.of();
                    auto const r = rhs_operand.of();
                    if (l == r) {
                        msg << "always";
                    } else {
                        msg << "never";
                    }
                    msg << " be equal to the right-hand side";

                    auto error =
                        viua::tooling::errors::compile_time::Error_wrapper{}
                            .append(viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::Useless_comparison,
                                line->tokens().at(0),
                                msg.str()});

                    {
                        auto const& definition_location =
                            function_state.defined_at(lhs_index,
                                                      lhs.register_set);
                        error.append(viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::
                                Compile_time_error::Empty_error,
                            definition_location.at(0)}
                                         .note("left-hand side defined here:"));
                    }
                    {
                        auto const& definition_location =
                            function_state.defined_at(rhs_index,
                                                      rhs.register_set);
                        error.append(
                            viua::tooling::errors::compile_time::Error{
                                viua::tooling::errors::compile_time::
                                    Compile_time_error::Empty_error,
                                definition_location.at(0)}
                                .note("right-hand side defined here:"));
                    }

                    throw error;
                }

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());
                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Boolean>()),
                    std::move(defining_tokens));

                break;
            }
            case STRUCT:
            {
                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                function_state.define_register(
                    function_state.resolve_index(dest),
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Struct>()),
                    std::move(defining_tokens));

                break;
            }
            case STRUCTINSERT:
            {
                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());
                auto const dest_index = throw_if_empty(function_state, dest);
                auto const dest_type_signature = maybe_with_pointer(
                    dest.access, {values::Value_type::Struct});
                throw_if_invalid_type(
                    function_state, dest, dest_index, dest_type_signature);

                auto const& key = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const key_index = throw_if_empty(function_state, key);
                auto const key_type_signature =
                    maybe_with_pointer(key.access, {values::Value_type::Atom});
                throw_if_invalid_type(
                    function_state, key, key_index, key_type_signature);

                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());
                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Value});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                {
                    using values::Atom;
                    using values::Struct;

                    auto const& key_value = static_cast<const Atom&>(
                        function_state.type_of(key_index, key.register_set)
                            .value());
                    auto& struct_value = static_cast<Struct&>(
                        function_state.type_of(dest_index, dest.register_set)
                            .value());

                    if (key_value.known()) {
                        struct_value.field(
                            key_value.of(),
                            function_state.type_of(source_index,
                                                   source.register_set));
                    }
                }

                if (source.access
                    != viua::internals::Access_specifier::POINTER_DEREFERENCE) {
                    function_state.erase_register(
                        source_index,
                        source.register_set,
                        std::vector<viua::tooling::libs::lexer::Token>{
                            line->token(0)});
                }

                break;
            }
            case STRUCTREMOVE:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Struct});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const& key = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());
                auto const key_index = throw_if_empty(function_state, key);
                auto const key_type_signature =
                    maybe_with_pointer(key.access, {values::Value_type::Atom});
                throw_if_invalid_type(
                    function_state, key, key_index, key_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                using values::Atom;
                using values::Struct;

                auto const& key_value = static_cast<const Atom&>(
                    function_state.type_of(key_index, key.register_set)
                        .value());
                auto& struct_value = static_cast<Struct&>(
                    function_state.type_of(source_index, source.register_set)
                        .value());
                if (key_value.known()
                    and struct_value.field(key_value.of()).has_value()) {
                    function_state.define_register(
                        function_state.resolve_index(dest),
                        dest.register_set,
                        struct_value.field(key_value.of()).value(),
                        std::move(defining_tokens));
                } else {
                    function_state.define_register(
                        function_state.resolve_index(dest),
                        dest.register_set,
                        function_state.make_wrapper(
                            std::make_unique<values::Value>(
                                values::Value_type::Value)),
                        std::move(defining_tokens));
                }

                break;
            }
            case STRUCTAT:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Struct});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const& key = *static_cast<Register_address const*>(
                    instruction.operands.at(2).get());
                auto const key_index = throw_if_empty(function_state, key);
                auto const key_type_signature =
                    maybe_with_pointer(key.access, {values::Value_type::Atom});
                throw_if_invalid_type(
                    function_state, key, key_index, key_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                using values::Atom;
                using values::Struct;

                auto const& key_value = static_cast<const Atom&>(
                    function_state.type_of(key_index, key.register_set)
                        .value());
                auto& struct_value = static_cast<Struct&>(
                    function_state.type_of(source_index, source.register_set)
                        .value());
                if (key_value.known()
                    and struct_value.field(key_value.of()).has_value()) {
                    function_state.define_register(
                        function_state.resolve_index(dest),
                        dest.register_set,
                        function_state.make_wrapper(
                            std::make_unique<values::Pointer>(
                                struct_value.field(key_value.of()).value())),
                        std::move(defining_tokens));
                } else {
                    function_state.define_register(
                        function_state.resolve_index(dest),
                        dest.register_set,
                        function_state.make_wrapper(
                            std::make_unique<values::Pointer>(
                                function_state.make_wrapper(
                                    std::make_unique<values::Value>(
                                        values::Value_type::Value)))),
                        std::move(defining_tokens));
                }

                break;
            }
            case STRUCTKEYS:
            {
                auto const& source = *static_cast<Register_address const*>(
                    instruction.operands.at(1).get());
                auto const source_index =
                    throw_if_empty(function_state, source);
                auto const source_type_signature = maybe_with_pointer(
                    source.access, {values::Value_type::Struct});
                throw_if_invalid_type(function_state,
                                      source,
                                      source_index,
                                      source_type_signature);

                auto const& dest = *static_cast<Register_address const*>(
                    instruction.operands.at(0).get());

                auto defining_tokens =
                    std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                copy_whole(dest.tokens(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index,
                    dest.register_set,
                    function_state.make_wrapper(
                        std::make_unique<values::Vector>(
                            function_state.make_wrapper(
                                std::make_unique<values::Atom>()))),
                    std::move(defining_tokens));

                break;
            }
            case EXCEPTION:
            case EXCEPTION_TAG:
            case EXCEPTION_VALUE:
            {
                // FIXME add static analysis for exception instructions
                break;
            }
            case IO_READ:
            case IO_WRITE:
            case IO_CLOSE:
            case IO_WAIT:
            case IO_CANCEL:
            {
                // FIXME add static analysis for I/O instructions
                break;
            }
            case RETURN:
            case HALT:
            {
                /*
                 * These instructions just cause the analysis to stop as they
                 * halt function's execution.
                 */
                return arm_result;
            }
            default:
            {
                // FIXME do nothing, but later add a warning that such-and-such
                // instruction is not covered by static analyser
                break;
            }
            }
        }

        function_state.dump(std::cout);
    }

    return arm_result;
}

static auto analyse_single_function(
    viua::tooling::libs::parser::Cooked_function const& fn,
    viua::tooling::libs::parser::Cooked_fragments const& fragments,
    Analyser_state& as) -> void
{
    using viua::tooling::errors::compile_time::Compile_time_error;

    if (fn.body().size() == 0) {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                Compile_time_error::Empty_function_body,
                fn.head().token(fn.head().tokens().size() - 3),
                "of " + fn.head().function_name})
            .append(viua::tooling::errors::compile_time::Error{
                Compile_time_error::Empty_error,
                fn.head().token(fn.head().tokens().size() - 3)}
                        .note("a function body must be composed of at least "
                              "two instructions:")
                        .comment("    allocate_registers %0 local")
                        .comment("    return"));
    }

    using viua::tooling::libs::parser::Fragment_type;
    using viua::tooling::libs::parser::Instruction;
    if (auto const& l = *fn.body().at(0);
        not(l.type() == Fragment_type::Instruction
            and static_cast<Instruction const&>(l).opcode
                    == ALLOCATE_REGISTERS)) {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                Compile_time_error::Must_allocate_registers_first, l.token(0)}
                .note("the first instruction of every function must be "
                      "`allocate_registers'"));
    }

    auto const& body = fn.body();
    using viua::tooling::libs::parser::Register_address;
    auto function_state = Function_state{
        static_cast<Register_address const*>(
            static_cast<Instruction const*>(body.at(0))->operands.at(0).get())
            ->index,
        body.at(0)->tokens()};

    auto const analysed_function_name =
        fn.head().function_name + '/' + std::to_string(fn.head().arity);

    std::cout << "analyse_single_function(): " << analysed_function_name;
    std::cout << " (" << fn.body().size() << " lines)";
    std::cout << std::endl;

    if (fn.head().function_name == "main") {
        if (fn.head().arity == 0) {
            // no arguments passed
        } else if (fn.head().arity == 1) {
            function_state.define_register(
                0,
                viua::bytecode::codec::Register_set::Parameters,
                function_state.make_wrapper(std::make_unique<values::Vector>(
                    function_state.make_wrapper(
                        std::make_unique<values::String>()))),
                fn.head().tokens());
        } else if (fn.head().arity == 2) {
            function_state.define_register(
                0,
                viua::bytecode::codec::Register_set::Parameters,
                function_state.make_wrapper(std::make_unique<values::String>()),
                fn.head().tokens());
            function_state.define_register(
                1,
                viua::bytecode::codec::Register_set::Parameters,
                function_state.make_wrapper(std::make_unique<values::Vector>(
                    function_state.make_wrapper(
                        std::make_unique<values::String>()))),
                fn.head().tokens());
        } else {
            using viua::tooling::errors::compile_time::Compile_time_error;
            auto error =
                viua::tooling::errors::compile_time::Error_wrapper{}.append(
                    viua::tooling::errors::compile_time::Error{
                        Compile_time_error::Empty_error,
                        fn.head().tokens().at(0),
                        "arity " + std::to_string(fn.head().arity)
                            + " is not value for the main function"}
                        .note("only arities 0, 1, and 2 are valid for main "
                              "function"));
            throw error;
        }
    } else {
        using arity_type = viua::bytecode::codec::register_index_type;
        for (auto i = arity_type{0}; i < fn.head().arity; ++i) {
            function_state.define_register(
                i,
                viua::bytecode::codec::Register_set::Parameters,
                function_state.make_wrapper(
                    std::make_unique<values::Value>(values::Value_type::Value)),
                fn.head().tokens());
        }
    }

    function_state.dump(std::cout);

    auto const annotated_body = annotate_body(body);
    auto const label_map      = create_label_map(body, annotated_body);

    analyse_single_arm(fn,
                       std::nullopt,
                       fragments,
                       as,
                       function_state,
                       false,
                       annotated_body,
                       label_map,
                       0,
                       false);

    if (fn.head().function_name == "main") {
        auto target =
            Register_address{0,
                             false,
                             false,
                             viua::bytecode::codec::Register_set::Local,
                             viua::internals::Access_specifier::DIRECT};
        target.add(fn.head().tokens().at(0));
        target.add(fn.head().tokens().at(0));
        target.add(fn.head().tokens().at(0));

        try {
            auto const target_index = throw_if_empty(function_state, target);

            auto const target_access = target.access;
            auto const target_type_signature =
                (target_access
                 == viua::internals::Access_specifier::POINTER_DEREFERENCE)
                    ? std::vector<
                        values::Value_type>{values::Value_type::Pointer,
                                            values::Value_type::Integer}
                    : std::vector<values::Value_type>{
                        values::Value_type::Integer};
            if (not function_state.assume_type(
                    target_index, target.register_set, target_type_signature)) {
                auto error =
                    viua::tooling::errors::compile_time::Error_wrapper{}.append(
                        viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::
                                Compile_time_error::Type_mismatch,
                            target.tokens().at(0),
                            "expected `" + to_string(target_type_signature)
                                + "'..."}
                            .add(target.tokens().at(0)));

                auto const& definition_location = function_state.defined_at(
                    target_index, target.register_set);
                error.append(viua::tooling::errors::compile_time::Error{
                    viua::tooling::errors::compile_time::Compile_time_error::
                        Empty_error,
                    definition_location.at(0),
                    ("...got `"
                     + to_string(function_state
                                     .type_of(target_index, target.register_set)
                                     .to_simple())
                     + "'")}
                                 .note("defined here"));
                throw error;
            }
        } catch (viua::tooling::errors::compile_time::Error_wrapper& e) {
            e.append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Empty_error,
                fn.head().token(0),
                "when checking return value of main function"});
            throw;
        }
    }
}

static auto analyse_functions(
    viua::tooling::libs::parser::Cooked_fragments const& fragments,
    Analyser_state& analyser_state) -> void
{
    auto const& functions = fragments.function_fragments;

    if (not(functions.count("main/0") or functions.count("main/1")
            or functions.count("main/2"))) {
        using viua::tooling::errors::compile_time::Compile_time_error;
        auto error =
            viua::tooling::errors::compile_time::Error_wrapper{}.append(
                viua::tooling::errors::compile_time::Error{
                    Compile_time_error::No_main_function_defined,
                    viua::tooling::libs::lexer::Token()}
                    .note("one of the `main/0', `main/1', or `main/2' "
                          "functions must be defined"));
        throw error;
    }

    for (auto const& [name, fn] : functions) {
        try {
            analyse_single_function(fn, fragments, analyser_state);
        } catch (viua::tooling::errors::compile_time::Error_wrapper& e) {
            e.append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Empty_error,
                fn.head().token(0),
                "in function `" + name + "'"});
            throw;
        }
    }
}

auto analyse(viua::tooling::libs::parser::Cooked_fragments const& fragments)
    -> void
{
    auto analyser_state = Analyser_state{};

    for (auto const& [name, block] : fragments.block_fragments) {
        if (not block.body().size()) {
            auto error =
                viua::tooling::errors::compile_time::Error_wrapper{}.append(
                    viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Empty_block_body,
                        block.lines.at(0)->tokens().at(1)});
            throw error;
        }
    }

    analyse_functions(fragments, analyser_state);

    /* analyse_closure_instantiations(fragments); */
}

}}}}  // namespace viua::tooling::libs::static_analyser
