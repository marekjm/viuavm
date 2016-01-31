#include <string>
#include <tuple>
#include <viua/support/string.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/program.h>
using namespace std;


int_op assembler::operands::getint(const string& s) {
    if (s.size() == 0) {
        throw "empty string cannot be used as operand";
    }
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

tuple<string, string> assembler::operands::get2(string s) {
    /** Returns tuple of two strings - two operands chunked from the `s` string.
     */
    string op_a, op_b;
    op_a = str::chunk(s);
    s = str::sub(s, op_a.size());
    op_b = str::chunk(s);
    return tuple<string, string>(op_a, op_b);
}

tuple<string, string, string> assembler::operands::get3(string s, bool fill_third) {
    string op_a, op_b, op_c;

    op_a = str::chunk(s);
    s = str::lstrip(str::sub(s, op_a.size()));

    op_b = str::chunk(s);
    s = str::lstrip(str::sub(s, op_b.size()));

    /* If s is empty and fill_third is true, use first operand as a filler.
     * In any other case, use the chunk of s.
     * The chunk of empty string will give us empty string and
     * it is a valid (and sometimes wanted) value to return.
     */
    op_c = (s.size() == 0 and fill_third ? op_a : str::chunk(s));

    return tuple<string, string, string>(op_a, op_b, op_c);
}
