#ifndef VIUA_ASSEMBLER_H
#define VIUA_ASSEMBLER_H

#pragma once


#include <string>
#include <vector>
#include <tuple>
#include <map>
#include "../program.h"

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
        std::map<std::string, std::pair<bool, std::vector<std::string> > > getFunctions(const std::vector<std::string>& lines);
        std::vector<std::string> getBlockNames(const std::vector<std::string>& lines);
        std::map<std::string, std::vector<std::string> > getBlocks(const std::vector<std::string>& lines);
    }

    namespace verify {
        std::string functionCalls(const std::vector<std::string>& lines, const std::vector<std::string>& function_names);
        std::string callableCreations(const std::vector<std::string>& lines, const std::vector<std::string>& function_names);
        std::string ressInstructions(const std::vector<std::string>& lines, bool as_lib);
        std::string functionBodiesAreNonempty(const std::vector<std::string>& lines, std::map<std::string, std::pair<bool, std::vector<std::string> > >& functions);
        std::string blockTries(const std::vector<std::string>& lines, const std::vector<std::string>& block_names);
        std::string blockBodiesEndWithLeave(const std::vector<std::string>& lines, std::map<std::string, std::pair<bool, std::vector<std::string> > >& blocks);
    }
}


#endif
