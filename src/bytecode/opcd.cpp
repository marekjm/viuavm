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

#include <iostream>
#include <algorithm>
#include <map>
#include <string>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/maps.h>
using namespace std;


int main() {
    long unsigned max_mnemonic_length = 0;
    for (pair<const OPCODE, string> i : OP_NAMES) {
        max_mnemonic_length = ((max_mnemonic_length >= i.second.size()) ? max_mnemonic_length : i.second.size());
    }

    // separate mnemonic from the rest of data
    max_mnemonic_length += 4;

    const string initial_column = "MNEMONIC    ";
    cout << initial_column << "| OPCODE | HEX OPCODE | SIZE | LENGTH\n" << endl;

    max_mnemonic_length = (max_mnemonic_length < initial_column.size() ? initial_column.size() : max_mnemonic_length);

    for (pair<const OPCODE, string> i : OP_NAMES) {
        cout << i.second;
        for (long unsigned j = i.second.size(); j < (max_mnemonic_length); ++j) {
            cout << ' ';
        }
        cout << "  ";
        cout << i.first << (i.first < 10 ? " " : "");
        cout << "       ";
        cout << "0x" << hex << i.first << (i.first < 0x10 ? " " : "") << dec;
        cout << "         ";
        cout << OP_SIZES.at(i.second) << (OP_SIZES.at(i.second) < 10 ? " " : "");
        cout << "     ";
        cout << ((find(OP_VARIABLE_LENGTH.begin(), OP_VARIABLE_LENGTH.end(), i.first) != OP_VARIABLE_LENGTH.end()) ? "variable" : "constant");
        cout << '\n';
    }
    cout << flush;
    return 0;
}
