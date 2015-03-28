#include <string>
#include <tuple>
#include "../support/string.h"
#include "../program.h"
#include "assembler.h"
using namespace std;


int_op assembler::operands::getint(const string& s) {
    bool ref = s[0] == '@';
    return tuple<bool, int>(ref, stoi(ref ? str::sub(s, 1) : s));
}

byte_op assembler::operands::getbyte(const string& s) {
    bool ref = s[0] == '@';
    return tuple<bool, char>(ref, (char)stoi(ref ? str::sub(s, 1) : s));
}

float_op assembler::operands::getfloat(const string& s) {
    bool ref = s[0] == '@';
    return tuple<bool, float>(ref, stof(ref ? str::sub(s, 1) : s));
}
