#ifndef VIUA_ASSEMBLER_H
#define VIUA_ASSEMBLER_H

#pragma once


#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <unordered_set>
#include <regex>
#include "../../program.h"

namespace assembler {
    namespace operands {
        int_op getint(const std::string& s);
        byte_op getbyte(const std::string& s);
        float_op getfloat(const std::string& s);

        std::tuple<std::string, std::string> get2(std::string s);
        std::tuple<std::string, std::string, std::string> get3(std::string s, bool fill_third = true);
    }

    namespace ce {
        std::vector<std::string> getilines(const std::vector<std::string>& lines);

        std::map<std::string, int> getmarks(const std::vector<std::string>& lines);
        std::map<std::string, int> getnames(const std::vector<std::string>& lines);
        std::vector<std::string> getlinks(const std::vector<std::string>& lines);

        std::vector<std::string> getFunctionNames(const std::vector<std::string>& lines);
        std::vector<std::string> getSignatures(const std::vector<std::string>& lines);
        std::vector<std::string> getBlockNames(const std::vector<std::string>& lines);
        std::vector<std::string> getBlockSignatures(const std::vector<std::string>& lines);
        std::map<std::string, std::vector<std::string> > getInvokables(const std::string& type, const std::vector<std::string>& lines);
        std::unordered_set<std::string> getTypeNames(const std::vector<std::string>&);
    }

    namespace verify {
        std::string functionNames(const std::string&, const std::vector<std::string>&, const bool, const bool);
        std::string functionsEndWithReturn(const std::string&, const std::vector<std::string>&, const bool, const bool);
        std::string functionCallsAreDefined(const std::string&, const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&, const std::vector<std::string>& function_names, const std::vector<std::string>& function_signatures);
        std::string functionCallArities(const std::string&, const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&, bool);
        std::string frameBalance(const std::string&, const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&);
        std::string callableCreations(const std::string&, const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&, const std::vector<std::string>& function_names, const std::vector<std::string>& function_signatures);
        std::string ressInstructions(const std::string&, const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&, bool as_lib);
        std::string functionBodiesAreNonempty(std::map<std::string, std::vector<std::string> >& functions);
        std::string blockBodiesAreNonempty(const std::string&, const std::vector<std::string>&);
        std::string blockTries(const std::string&, const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&, const std::vector<std::string>& block_names, const std::vector<std::string>& block_signatures);
        std::string blockBodiesEndWithLeave(const std::vector<std::string>& lines, std::map<std::string, std::pair<bool, std::vector<std::string> > >& blocks);
        std::string mainFunctionDoesNotEndWithHalt(std::map<std::string, std::vector<std::string> >& functions);

        std::string directives(const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&);
        std::string instructions(const std::vector<std::string>& lines, const std::map<long unsigned, long unsigned>&);
    }

    namespace utils {
        std::regex getFunctionNameRegex();
        bool isValidFunctionName(const std::string&);
        std::smatch matchFunctionName(const std::string&);
        int getFunctionArity(const std::string&);
    }
}


#endif
