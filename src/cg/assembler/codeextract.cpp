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

#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <viua/support/string.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/program.h>
using namespace std;


vector<string> assembler::ce::getilines(const vector<string>& lines) {
    /*  Clears code from empty lines and comments.
     */
    vector<string> ilines;
    string line;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (!line.size() or line[0] == ';' or str::startswith(line, "--")) continue;
        ilines.emplace_back(line);
    }

    return ilines;
}

map<string, int> assembler::ce::getmarks(const vector<string>& lines) {
    /** This function will pass over all instructions and
     * gather "marks", i.e. `.mark: <name>` directives which may be used by
     * `jump` and `branch` instructions.
     *
     * When referring to a mark in code, you should use: `jump :<name>`.
     *
     * The colon before name of the marker is placed here to make it possible to use numeric markers
     * which would otherwise be treated as instruction indexes.
     */
    map<string, int> marks;
    string line, mark;
    int instruction = 0;  // we need separate instruction counter because number of lines is not exactly number of instructions
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = lines[i];
        if (assembler::utils::lines::is_name(line) or assembler::utils::lines::is_link(line)) {
            // names and links can be safely skipped as they are not CPU instructions
            continue;
        }
        if (assembler::utils::lines::is_function(line)) {
            // instructions in functions are counted separately so they are
            // not included here
            while (!str::startswith(lines[i], ".end")) { ++i; }
            continue;
        }
        if (not assembler::utils::lines::is_mark(line)) {
            // if all previous checks were false, then this line must be either .mark: directive or
            // an instruction
            // if this check is true - then it is an instruction
            ++instruction;
            continue;
        }

        // get mark name
        line = str::lstrip(str::sub(line, 6));
        mark = str::chunk(line);

        // create mark for current instruction
        marks[mark] = instruction;
    }
    return marks;
}

map<string, int> assembler::ce::getnames(const vector<string>& lines) {
    /** This function will pass over all instructions and
     *  gather "names", i.e. `.name: <register> <name>` instructions which may be used by
     *  as substitutes for register indexes to more easily remember what is stored where.
     *
     *  Example name instruction: `.name: 1 base`.
     *  This allows to access first register with name `base` instead of its index.
     *
     *  Example (which also uses marks) name reference could be: `branch if_equals_0 :finish`.
     */
    map<string, int> names;
    string line, reg, name;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = lines[i];
        if (not assembler::utils::lines::is_name(line)) {
            continue;
        }

        line = str::lstrip(str::sub(line, 6));
        reg = str::chunk(line);
        line = str::lstrip(str::sub(line, reg.size()));
        name = str::chunk(line);

        try {
            names[name] = stoi(reg);
        } catch (const std::invalid_argument& e) {
            throw "invalid register index in .name instruction";
        }
    }
    return names;
}

vector<string> assembler::ce::getlinks(const vector<string>& lines) {
    /** This function will pass over all instructions and
     * gather .link: assembler instructions.
     */
    vector<string> links;
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = lines[i];
        if (assembler::utils::lines::is_link(line)) {
            line = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
            links.emplace_back(line);
        }
    }
    return links;
}

vector<string> assembler::ce::getFunctionNames(const vector<string>& lines) {
    vector<string> names;

    string line, holdline;
    for (unsigned i = 0; i < lines.size(); ++i) {
        holdline = line = lines[i];
        if (not assembler::utils::lines::is_function(line)) { continue; }

        if (assembler::utils::lines::is_function(line)) {
            for (unsigned j = i+1; not assembler::utils::lines::is_end(lines[j]); ++j, ++i) {}
        }

        line = str::lstrip(str::sub(line, str::chunk(line).size()));
        names.emplace_back(str::chunk(line));
    }

    return names;
}
vector<string> assembler::ce::getSignatures(const vector<string>& lines) {
    vector<string> names;

    string line, holdline;
    for (unsigned i = 0; i < lines.size(); ++i) {
        holdline = line = lines[i];
        if (not assembler::utils::lines::is_function_signature(line)) { continue; }
        line = str::lstrip(str::sub(line, str::chunk(line).size()));
        names.emplace_back(str::chunk(line));
    }

    return names;
}
vector<string> assembler::ce::getBlockNames(const vector<string>& lines) {
    vector<string> names;

    string line, holdline;
    for (unsigned i = 0; i < lines.size(); ++i) {
        holdline = line = lines[i];
        if (not assembler::utils::lines::is_block(line)) { continue; }

        if (assembler::utils::lines::is_block(line)) {
            for (unsigned j = i+1; not assembler::utils::lines::is_end(lines[j]); ++j, ++i) {}
        }

        line = str::lstrip(str::sub(line, str::chunk(line).size()));
        string name = str::chunk(line);

        names.emplace_back(name);
    }

    return names;
}
vector<string> assembler::ce::getBlockSignatures(const vector<string>& lines) {
    vector<string> names;

    string line, holdline;
    for (unsigned i = 0; i < lines.size(); ++i) {
        holdline = line = lines[i];
        if (not assembler::utils::lines::is_block_signature(line)) { continue; }
        line = str::lstrip(str::sub(line, str::chunk(line).size()));
        names.emplace_back(str::chunk(line));
    }

    return names;
}

map<string, vector<string> > assembler::ce::getInvokables(const string& type, const vector<string>& lines) {
    map<string, vector<string> > invokables;

    string opening;
    if (type == "function") {
        opening = ".function:";
    } else if (type == "block") {
        opening = ".block:";
    }

    string line, holdline;
    for (unsigned i = 0; i < lines.size(); ++i) {
        holdline = line = lines[i];
        if (!str::startswith(line, opening)) { continue; }

        vector<string> flines;
        for (unsigned j = i+1; lines[j] != ".end"; ++j, ++i) {
            if (str::startswith(lines[j], opening)) {
                throw ("another " + type + " opened before assembler reached .end after '" + str::chunk(str::sub(holdline, str::chunk(holdline).size())) + "' " + type);
            }
            flines.emplace_back(lines[j]);
        }

        line = str::lstrip(str::sub(line, opening.size()));
        string name = str::chunk(line);
        line = str::lstrip(str::sub(line, name.size()));

        invokables[name] = flines;
    }

    return invokables;
}
