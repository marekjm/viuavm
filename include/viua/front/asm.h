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

#ifndef VIUA_FRONT_ASM_H
#define VIUA_FRONT_ASM_H

#pragma once

#include <string>
#include <vector>
#include <map>


struct invocables_t {
    std::vector<std::string> names;
    std::vector<std::string> signatures;
    std::map<std::string, std::vector<std::string>> bodies;
};

struct compilationflags_t {
    bool as_lib;

    bool verbose;
    bool debug;
    bool scream;
};

struct srcline_t {
    std::string line;
    unsigned expanded_from;

    srcline_t(const std::string& l, unsigned ef): line(l), expanded_from(ef) {}
};


std::vector<std::string> expandSource(const std::vector<std::string>&, std::map<long unsigned, long unsigned>&);

std::vector<std::vector<std::string>> decode_line_tokens(const std::vector<std::string>&);
std::vector<std::vector<std::string>> decode_line(const std::string&);

int gatherFunctions(invocables_t*, const std::vector<std::string>&, const std::vector<std::string>&);
int gatherBlocks(invocables_t*, const std::vector<std::string>&, const std::vector<std::string>&);

int generate(const std::vector<std::string>&, const std::map<long unsigned, long unsigned>&, std::vector<std::string>&, invocables_t&, invocables_t&, const std::string&, std::string&, const std::vector<std::string>&, const compilationflags_t&);


#endif
