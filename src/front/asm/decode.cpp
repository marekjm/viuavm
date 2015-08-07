#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_set>
#include <viua/bytecode/maps.h>
#include <viua/support/string.h>
#include <viua/support/env.h>
#include <viua/loader.h>
#include <viua/program.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/front/asm.h>
using namespace std;


vector<string> tokenize(const string& s) {
    vector<string> tokens;
    ostringstream token;
    token.str("");
    for (unsigned i = 0; i < s.size(); ++i) {
        if (s[i] == ' ' and token.str().size()) {
            tokens.push_back(token.str());
            token.str("");
            continue;
        }
        if (s[i] == ' ') {
            continue;
        }
        if (s[i] == '^') {
            if (token.str().size()) {
                tokens.push_back(token.str());
                token.str("");
            }
            tokens.push_back("^");
        }
        if (s[i] == '(' or s[i] == ')') {
            if (token.str().size()) {
                tokens.push_back(token.str());
                token.str("");
            }
            tokens.push_back((s[i] == '(' ? "(" : ")"));
            continue;
        }
        if (s[i] == '[' or s[i] == ']') {
            if (token.str().size()) {
                tokens.push_back(token.str());
                token.str("");
            }
            tokens.push_back((s[i] == '[' ? "[" : "]"));
            continue;
        }
        if (s[i] == '{' or s[i] == '}') {
            if (token.str().size()) {
                tokens.push_back(token.str());
                token.str("");
            }
            tokens.push_back((s[i] == '{' ? "{" : "}"));
            continue;
        }
        if (s[i] == '"' or s[i] == '\'') {
            string ss = str::extract(s.substr(i));
            i += (ss.size()-1);
            tokens.push_back(ss);
            continue;
        }
        token << s[i];
    }
    if (token.str().size()) {
        tokens.push_back(token.str());
    }
    return tokens;
}

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
