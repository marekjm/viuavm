#include <sstream>
#include <viua/support/string.h>
#include <viua/cg/tokenizer.h>
#include <viua/front/asm.h>
using namespace std;


vector<vector<string>> decode_line_tokens(const vector<string>& tokens) {
    vector<vector<string>> decoded_lines;
    vector<string> main_line;

    unsigned i = 0;
    bool invert = false;
    while (i < tokens.size()) {
        if (tokens[i] == "^") {
            invert = true;
            ++i;
            continue;
        }
        if (tokens[i] == "(") {
            vector<string> subtokens;
            ++i;
            int balance = 1;
            while (i < tokens.size()) {
                if (tokens[i] == "(") { ++balance; }
                if (tokens[i] == ")") { --balance; }
                if (balance == 0) { break; }
                subtokens.push_back(tokens[i]);
                ++i;
            }
            vector<vector<string>> sublines = decode_line_tokens(subtokens);
            for (unsigned j = 0; j < sublines.size(); ++j) {
                decoded_lines.push_back(sublines[j]);
            }
            main_line.push_back(sublines[sublines.size()-1][1]);
            ++i;
            continue;
        }
        if (tokens[i] == "[") {
            vector<string> subtokens;
            ++i;
            int balance = 1;
            int toplevel_subexpr_balance = 0;
            unsigned len = 0;
            while (i < tokens.size()) {
                if (tokens[i] == "[") { ++balance; }
                if (tokens[i] == "]") { --balance; }
                if (tokens[i] == "(") {
                    if ((++toplevel_subexpr_balance) == 1) { ++len; }
                }
                if (tokens[i] == ")") {
                    --toplevel_subexpr_balance;
                }
                if (balance == 0) { break; }
                subtokens.push_back(tokens[i]);
                ++i;
            }
            vector<vector<string>> sublines = decode_line_tokens(subtokens);
            sublines.pop_back();
            for (unsigned j = 0; j < sublines.size(); ++j) {
                decoded_lines.push_back(sublines[j]);
            }
            main_line.push_back(str::stringify(len));
            ++i;
            continue;
        }
        main_line.push_back(tokens[i]);
        ++i;
    }

    if (invert) {
        decoded_lines.insert(decoded_lines.begin(), main_line);
    } else {
        decoded_lines.push_back(main_line);
    }

    return decoded_lines;
}
vector<vector<string>> decode_line(const string& s) {
    return decode_line_tokens(tokenize(s));
}
