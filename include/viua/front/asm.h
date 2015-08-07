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


std::vector<std::string> expandSource(const std::vector<std::string>&);
int gatherFunctions(invocables_t*, const std::vector<std::string>&, const std::vector<std::string>&);
int gatherBlocks(invocables_t*, const std::vector<std::string>&, const std::vector<std::string>&);
int generate(const std::vector<std::string>&, std::vector<std::string>&, invocables_t&, invocables_t&, std::string&, std::string&, const std::vector<std::string>&);


std::vector<std::string> tokenize(const std::string&);
std::vector<std::vector<std::string>> decode_line_tokens(const std::vector<std::string>&);
std::vector<std::vector<std::string>> decode_line(const std::string&);


#endif
