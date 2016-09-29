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
#include <viua/cg/lex.h>
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

        std::map<std::string, int> getmarks(const std::vector<viua::cg::lex::Token>&);
        std::map<std::string, int> getnames(const std::vector<viua::cg::lex::Token>&);
        std::vector<std::string> getlinks(const std::vector<viua::cg::lex::Token>&);

        std::vector<std::string> getFunctionNames(const std::vector<viua::cg::lex::Token>&);
        std::vector<std::string> getSignatures(const std::vector<viua::cg::lex::Token>&);
        std::vector<std::string> getBlockNames(const std::vector<viua::cg::lex::Token>&);
        std::vector<std::string> getBlockSignatures(const std::vector<viua::cg::lex::Token>&);
        std::map<std::string, std::vector<std::string> > getInvokables(const std::string& type, const std::vector<viua::cg::lex::Token>&);
        std::map<std::string, std::vector<viua::cg::lex::Token>> getInvokablesTokenBodies(const std::string&, const std::vector<viua::cg::lex::Token>&);
    }

    namespace verify {
        void functionCallsAreDefined(const std::vector<viua::cg::lex::Token>&, const std::vector<std::string>&, const std::vector<std::string>&);

        void functionCallArities(const std::vector<std::string>&);
        void msgArities(const std::vector<std::string>& lines);

        void functionNames(const std::vector<viua::cg::lex::Token>&);
        void functionsEndWithReturn(const std::vector<std::string>&);

        void frameBalance(const std::vector<viua::cg::lex::Token>&);

        void blockTries(const std::vector<viua::cg::lex::Token>&, const std::vector<std::string>&, const std::vector<std::string>&);
        void blockCatches(const std::vector<viua::cg::lex::Token>&, const std::vector<std::string>&, const std::vector<std::string>&);

        void callableCreations(const std::vector<viua::cg::lex::Token>&, const std::vector<std::string>&, const std::vector<std::string>&);

        void ressInstructions(const std::vector<viua::cg::lex::Token>&, bool);

        void functionBodiesAreNonempty(const std::vector<viua::cg::lex::Token>&);
        void blockBodiesAreNonempty(const std::vector<viua::cg::lex::Token>&);

        void blocksEndWithFinishingInstruction(const std::vector<std::string>&);

        void directives(const std::vector<viua::cg::lex::Token>&);
        void instructions(const std::vector<viua::cg::lex::Token>&);

        void framesHaveOperands(const std::vector<std::string>&);
        void framesHaveNoGaps(const std::vector<std::string>&, const std::map<unsigned long, unsigned long>&);

        void jumpsAreInRange(const std::vector<std::string>&);

        void manipulationOfDefinedRegisters(const std::vector<viua::cg::lex::Token>&, const std::map<std::string, std::vector<viua::cg::lex::Token>>&, const bool);
    }

    namespace utils {
        std::regex getFunctionNameRegex();
        bool isValidFunctionName(const std::string&);
        std::smatch matchFunctionName(const std::string&);
        int getFunctionArity(const std::string&);

        namespace lines {
            bool is_directive(const std::string&);

            bool is_function(const std::string&);
            bool is_closure(const std::string&);
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
