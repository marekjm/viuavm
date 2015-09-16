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
