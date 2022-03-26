/*
 *  Copyright (C) 2022 Marek Marecki
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

#ifndef VIUA_LIBS_STAGE_H
#define VIUA_LIBS_STAGE_H

#include <sys/types.h>

#include <vector>
#include <string_view>

#include <viua/libs/errors/compile_time.h>

namespace viua::libs::stage {
auto view_line_of(std::string_view sv, viua::libs::lexer::Location loc)
    -> std::string_view;
auto view_line_before(std::string_view sv, viua::libs::lexer::Location loc)
    -> std::string_view;
auto view_line_after(std::string_view sv, viua::libs::lexer::Location loc)
    -> std::string_view;

auto cook_spans(
    std::vector<viua::libs::errors::compile_time::Error::span_type> raw)
    -> std::vector<std::tuple<bool, size_t, size_t>>;
auto display_error_and_exit
    [[noreturn]] (std::filesystem::path source_path,
                  std::string_view source_text,
                  viua::libs::errors::compile_time::Error const& e) -> void;

auto display_error_in_function(std::filesystem::path const source_path,
                               viua::libs::errors::compile_time::Error const& e,
                               std::string_view const fn_name) -> void;
}  // namespace viua::libs::stage

#endif
