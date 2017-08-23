/*
 *  Copyright (C) 2017 Marek Marecki
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

#ifndef VIUA_ASSEMBLER_FRONTEND_STATIC_ANALYSER_H
#define VIUA_ASSEMBLER_FRONTEND_STATIC_ANALYSER_H


#include <string>
#include <viua/assembler/frontend/parser.h>


namespace viua {
    namespace assembler {
        namespace frontend {
            namespace static_analyser {
                auto verify_ress_instructions(const parser::ParsedSource&) -> void;
                auto verify_block_tries(const parser::ParsedSource&) -> void;
                auto verify_block_catches(const parser::ParsedSource&) -> void;
                auto verify_block_endings(const parser::ParsedSource& src) -> void;
                auto verify_frame_balance(const parser::ParsedSource& src) -> void;
                auto verify_function_call_arities(const parser::ParsedSource& src) -> void;
                auto verify_frames_have_no_gaps(const parser::ParsedSource& src) -> void;
                auto verify_jumps_are_in_range(const parser::ParsedSource&) -> void;

                auto verify(const parser::ParsedSource&) -> void;
            }
        }
    }
}

#endif
