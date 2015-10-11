#ifndef VIUA_LOADER_H
#define VIUA_LOADER_H

#pragma once


#include <cstdint>
#include <fstream>
#include <tuple>
#include <string>
#include <vector>
#include <map>
#include <viua/bytecode/bytetypedef.h>

typedef std::tuple<std::vector<std::string>, std::map<std::string, uint16_t> > IdToAddressMapping;

class Loader {
    std::string path;

    uint64_t size;
    byte* bytecode;

    std::vector<uint64_t> jumps;

    std::map<std::string, uint64_t> function_addresses;
    std::map<std::string, uint64_t> function_sizes;
    std::vector<std::string> functions;
    std::map<std::string, uint64_t> block_addresses;
    std::vector<std::string> blocks;

    IdToAddressMapping loadmap(char*, const uint64_t&);
    void calculateFunctionSizes();

    void loadJumpTable(std::ifstream&);
    void loadFunctionsMap(std::ifstream&);
    void loadBlocksMap(std::ifstream&);
    void loadBytecode(std::ifstream&);

    public:
    Loader& load();
    Loader& executable();

    uint64_t getBytecodeSize();
    byte* getBytecode();

    std::vector<uint64_t> getJumps();

    std::map<std::string, uint64_t> getFunctionAddresses();
    std::map<std::string, uint64_t> getFunctionSizes();
    std::vector<std::string> getFunctions();

    std::map<std::string, uint64_t> getBlockAddresses();
    std::vector<std::string> getBlocks();

    Loader(std::string pth): path(pth), size(0), bytecode(nullptr) {}
    ~Loader() {
        delete[] bytecode;
    }
};


#endif
