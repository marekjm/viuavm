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

#include <elf.h>
#include <stdint.h>
#include <sys/types.h>

#include <map>
#include <string_view>
#include <vector>

#include <viua/libs/errors/compile_time.h>
#include <viua/libs/parser.h>

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

auto display_error(std::filesystem::path,
                   std::string_view,
                   viua::libs::errors::compile_time::Error const&) -> void;
auto display_error_and_exit
    [[noreturn]] (std::filesystem::path,
                  std::string_view,
                  viua::libs::errors::compile_time::Error const&) -> void;

auto display_error_in_function(std::filesystem::path const source_path,
                               viua::libs::errors::compile_time::Error const& e,
                               std::string_view const fn_name) -> void;

auto save_string_to_strtab(std::vector<uint8_t>&, std::string_view const)
    -> size_t;
auto save_buffer_to_rodata(std::vector<uint8_t>&, std::string_view const)
    -> size_t;
auto record_symbol(std::string,
                   Elf64_Sym const,
                   std::vector<Elf64_Sym>& table,
                   std::map<std::string, size_t>& cache) -> size_t;

auto cook_long_immediates(viua::libs::parser::ast::Instruction,
                          std::vector<uint8_t>&,
                          std::vector<Elf64_Sym>&,
                          std::map<std::string, size_t>&)
    -> std::vector<viua::libs::parser::ast::Instruction>;

auto expand_delete(std::vector<viua::libs::parser::ast::Instruction>&,
                   viua::libs::parser::ast::Instruction const&) -> void;
auto expand_li(std::vector<viua::libs::parser::ast::Instruction>&,
               viua::libs::parser::ast::Instruction const&,
               bool const force_full = false) -> void;
auto expand_if(std::vector<viua::libs::parser::ast::Instruction>&,
               viua::libs::parser::ast::Instruction&,
               std::map<size_t, size_t>) -> void;
auto expand_pseudoinstructions(
    std::vector<viua::libs::parser::ast::Instruction>,
    std::map<std::string, size_t> const& symbol_map)
    -> std::vector<viua::libs::parser::ast::Instruction>;

auto emit_instruction(viua::libs::parser::ast::Instruction const)
    -> viua::arch::instruction_type;
}  // namespace viua::libs::stage

#endif
