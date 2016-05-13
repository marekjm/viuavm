#include <string>
#include <regex>
#include <viua/cg/assembler/assembler.h>
using namespace std;


regex assembler::utils::getFunctionNameRegex() {
    return regex{"(?:::)?[a-zA-Z_][a-zA-Z0-9_]*(?:::[a-zA-Z_][a-zA-Z0-9_]*)*(?:/([0-9]+))?"};
}

bool assembler::utils::isValidFunctionName(const string& function_name) {
    return regex_match(function_name, getFunctionNameRegex());
}

smatch assembler::utils::matchFunctionName(const string& function_name) {
    smatch parts;
    regex_match(function_name, parts, getFunctionNameRegex());
    return parts;
}

int assembler::utils::getFunctionArity(const string& function_name) {
    int arity = -1;
    auto parts = matchFunctionName(function_name);
    if (parts[1].str().size()) {
        ssub_match a = parts[1];
        arity = stoi(a.str());
    }
    return arity;
}
