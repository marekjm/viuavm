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

#include <stdlib.h>

#include <iomanip>
#include <iostream>
#include <map>
#include <string>

#include <viua/tooling/errors/compile_time.h>
#include <viua/util/string/escape_sequences.h>

namespace viua { namespace tooling { namespace errors { namespace compile_time {
std::map<Compile_time_error, std::string> compile_time_error_descriptions = {
    {Compile_time_error::Unknown_error, "unknown error"},
    {Compile_time_error::Empty_error, ""},
    {Compile_time_error::Unknown_option, "unknown option"},
    {Compile_time_error::No_input_file, "no input file"},
    {Compile_time_error::Not_a_file, "not a file"},
    {Compile_time_error::Unexpected_token, "unexpected token"},
    {Compile_time_error::Unexpected_newline, "unexpected newline"},
    {Compile_time_error::Invalid_access_type_specifier,
     "invalid access type specifier"},
    {Compile_time_error::Must_allocate_registers_first,
     "must allocate registers first"},
    {Compile_time_error::Iota_outside_of_allocated_range,
     "iota creates address outside of allocated range"},
    {Compile_time_error::Register_index_outside_of_allocated_range,
     "register index creates address outside of allocated range"},
    {Compile_time_error::Renaming_already_named_register,
     "renaming already named register"},
    {Compile_time_error::Reusing_register_name, "reusing register name"},
    {Compile_time_error::Read_from_empty_register, "read from empty register"},
    {Compile_time_error::Type_mismatch, "mismatched types"},
    {Compile_time_error::Empty_function_body, "empty function body"},
    {Compile_time_error::Useless_comparison, "useless comparison"},
    {Compile_time_error::Overwrite_of_unused_frame,
     "overwrite of unused frame"},
    {Compile_time_error::Call_with_empty_slot,
     "an argument slot is left empty"},
    {Compile_time_error::Argument_pass_without_a_frame,
     "argument pass without a frame"},
    {Compile_time_error::Argument_pass_overwrites,
     "argument pass overwrites a previous value"},
    {Compile_time_error::No_main_function_defined, "no main function defined"},
    {Compile_time_error::Call_without_a_frame, "call without a frame"},
    {Compile_time_error::Catch_without_a_catch_frame,
     "catch without a catch frame"},
    {Compile_time_error::Reference_to_undefined_block,
     "reference to undefined block"},
    {Compile_time_error::Empty_block_body, "empty block body"},
};

auto display_error(Compile_time_error const error_code) -> std::string
{
    if (error_code == Compile_time_error::Empty_error) {
        return "";
    }

    using viua::util::string::escape_sequences::ATTR_RESET;
    using viua::util::string::escape_sequences::COLOR_FG_RED;
    using viua::util::string::escape_sequences::send_escape_seq;

    std::ostringstream o;
    o << send_escape_seq(COLOR_FG_RED) << "EC";
    o << std::setw(4) << std::setfill('0') << static_cast<uint64_t>(error_code);
    o << send_escape_seq(ATTR_RESET);
    o << ": ";
    o << compile_time_error_descriptions.at(error_code);
    return o.str();
}

auto display_error_and_exit(Compile_time_error const error_code) -> void
{
    using viua::util::string::escape_sequences::ATTR_RESET;
    using viua::util::string::escape_sequences::COLOR_FG_RED;
    using viua::util::string::escape_sequences::COLOR_FG_WHITE;
    using viua::util::string::escape_sequences::send_escape_seq;
    std::cerr << send_escape_seq(COLOR_FG_RED) << "error"
              << send_escape_seq(ATTR_RESET) << ": ";
    std::cerr << display_error(error_code);
    std::cerr << std::endl;
    exit(1);
}

auto display_error_and_exit(Compile_time_error const error_code,
                            std::string const message) -> void
{
    using viua::util::string::escape_sequences::ATTR_RESET;
    using viua::util::string::escape_sequences::COLOR_FG_RED;
    using viua::util::string::escape_sequences::COLOR_FG_WHITE;
    using viua::util::string::escape_sequences::send_escape_seq;
    std::cerr << send_escape_seq(COLOR_FG_RED) << "error"
              << send_escape_seq(ATTR_RESET) << ": ";
    std::cerr << display_error(error_code);
    std::cerr << ": " << message << std::endl;
    exit(1);
}
}}}}  // namespace viua::tooling::errors::compile_time
