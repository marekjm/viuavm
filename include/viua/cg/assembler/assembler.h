/*
 *  Copyright (C) 2015, 2016 Marek Marecki
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

#ifndef VIUA_ASSEMBLER_H
#define VIUA_ASSEMBLER_H

#pragma once


#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <regex>
#include <viua/program.h>

namespace assembler {
    namespace operands {
        int_op getint(const std::string& s);
        byte_op getbyte(const std::string& s);
        float_op getfloat(const std::string& s);

        std::tuple<std::string, std::string> get2(std::string s);
        std::tuple<std::string, std::string, std::string> get3(std::string s, bool fill_third = true);
    }

    namespace ce {
        std::vector<std::string> getilines(const std::vector<std::string>& lines);

        std::map<std::string, int> getmarks(const std::vector<std::string>& lines);
        std::map<std::string, int> getnames(const std::vector<std::string>& lines);
        std::vector<std::string> getlinks(const std::vector<std::string>& lines);

        std::vector<std::string> getFunctionNames(const std::vector<std::string>& lines);
        std::vector<std::string> getSignatures(const std::vector<std::string>& lines);
        std::vector<std::string> getBlockNames(const std::vector<std::string>& lines);
        std::vector<std::string> getBlockSignatures(const std::vector<std::string>& lines);
        std::map<std::string, std::vector<std::string> > getInvokables(const std::string& type, const std::vector<std::string>& lines);
    }

    namespace verify {
        std::string functionNames(const std::string&, const std::vector<std::string>&, const bool, const bool);
        std::string functionsEndWithReturn(const std::string&, const std::vector<std::string>&);
        std::string functionCallsAreDefined(const std::string&, const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&, const std::vector<std::string>& function_names, const std::vector<std::string>& function_signatures);
        std::string functionCallArities(const std::string&, const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&, bool);
        std::string msgArities(const std::string&, const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&, bool);
        std::string frameBalance(const std::string&, const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&);
        std::string callableCreations(const std::string&, const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&, const std::vector<std::string>& function_names, const std::vector<std::string>& function_signatures);
        std::string ressInstructions(const std::string&, const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&, bool as_lib);
        std::string functionBodiesAreNonempty(const std::string&, const std::vector<std::string>&);
        std::string blockBodiesAreNonempty(const std::string&, const std::vector<std::string>&);
        std::string blockTries(const std::string&, const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&, const std::vector<std::string>& block_names, const std::vector<std::string>& block_signatures);
        std::string blockCatches(const std::string&, const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&, const std::vector<std::string>& block_names, const std::vector<std::string>& block_signatures);
        std::string blockBodiesEndWithLeave(const std::vector<std::string>& lines, std::map<std::string, std::pair<bool, std::vector<std::string> > >& blocks);

        std::string directives(const std::string&, const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&);
        std::string instructions(const std::string&, const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&);

        std::string framesHaveNoGaps(const std::string&, const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&);
    }

    namespace utils {
        std::regex getFunctionNameRegex();
        bool isValidFunctionName(const std::string&);
        std::smatch matchFunctionName(const std::string&);
        int getFunctionArity(const std::string&);

        namespace lines {
            bool is_directive(const std::string&);

            bool is_function(const std::string&);
            bool is_block(const std::string&);
            bool is_function_signature(const std::string&);
            bool is_block_signature(const std::string&);
            bool is_name(const std::string&);
            bool is_mark(const std::string&);
            bool is_info(const std::string&);
            bool is_end(const std::string&);
            bool is_main(const std::string&);
            bool is_link(const std::string&);

            std::string make_function_signature(const std::string&);
            std::string make_block_signature(const std::string&);
            std::string make_info(const std::string&, const std::string&);
        }
    }
}


#endif
