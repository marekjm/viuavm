#include <sstream>
#include <viua/support/string.h>
#include <viua/cg/tokenizer.h>
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
