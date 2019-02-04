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

#ifndef VIUA_TOOLING_ERRORS_COMPILE_TIME_H
#define VIUA_TOOLING_ERRORS_COMPILE_TIME_H

#include <string>

namespace viua { namespace tooling { namespace errors { namespace compile_time {
enum class Compile_time_error {
    Unknown_error,
    Empty_error,
    Unknown_option,
    No_input_file,
    Not_a_file,
    Unexpected_token,
    Unexpected_newline,
    Invalid_access_type_specifier,
    Must_allocate_registers_first,
    Iota_outside_of_allocated_range,
    Register_index_outside_of_allocated_range,
    Renaming_already_named_register,
    Reusing_register_name,
    Read_from_empty_register,
    Type_mismatch,
    Empty_function_body,
    Useless_comparison,
    Overwrite_of_unused_frame,
    Call_with_empty_slot,
    Argument_pass_without_a_frame,
    Argument_pass_overwrites,
    No_main_function_defined,
    Call_without_a_frame,
};

auto display_error(Compile_time_error const) -> std::string;
auto display_error_and_exit(Compile_time_error const) -> void;
auto display_error_and_exit(Compile_time_error const, std::string const)
    -> void;
}}}}  // namespace viua::tooling::errors::compile_time

#endif
