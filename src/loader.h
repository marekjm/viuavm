#ifndef VIUA_LOADER_H
#define VIUA_LOADER_H

#pragma once


#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "bytecode/bytetypedef.h"

class Loader {
    std::string path;

    uint16_t size;
    byte* bytecode;

    std::vector<unsigned> jumps;

    std::map<std::string, uint16_t> function_addresses;
    std::vector<std::string> functions;
    std::map<std::string, uint16_t> block_addresses;
    std::vector<std::string> blocks;

    std::map<std::string, uint16_t> loadmap(char*, const uint16_t&);

    void loadFunctionsMap(std::ifstream&);
    void loadBlocksMap(std::ifstream&);

    public:
    Loader& load();
    Loader& executable();

    uint16_t getBytecodeSize();
    byte* getBytecode();

    std::vector<unsigned> getJumps();

    std::map<std::string, uint16_t> getFunctionAddresses();
    std::vector<std::string> getFunctions();
    std::map<std::string, uint16_t> getBlockAddresses();
    std::vector<std::string> getBlocks();

    std::map<std::string, uint16_t> getBlockAddresses();
    std::vector<std::string> getBlocks();

    Loader(std::string pth): path(pth), bytecode(0) {}
    ~Loader() {
        delete[] bytecode;
    }
};


#endif
