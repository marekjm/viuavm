/*
 *  Copyright (C) 2015, 2016 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VIUA_LOADER_H
#define VIUA_LOADER_H

#pragma once


#include <cstdint>
#include <fstream>
#include <tuple>
#include <string>
#include <vector>
#include <map>
#include <viua/machine.h>
#include <viua/bytecode/bytetypedef.h>

typedef std::tuple<std::vector<std::string>, std::map<std::string, uint64_t> > IdToAddressMapping;

template<class T> void readinto(std::ifstream& in, T* object) {
    in.read(reinterpret_cast<char*>(object), sizeof(T));
}

class Loader {
    std::string path;

    uint64_t size;
    byte* bytecode;

    std::vector<uint64_t> jumps;

    std::map<std::string, std::string> meta_information;

    std::vector<std::string> external_signatures;
    std::vector<std::string> external_signatures_block;

    std::map<std::string, uint64_t> function_addresses;
    std::map<std::string, uint64_t> function_sizes;
    std::vector<std::string> functions;
    std::map<std::string, uint64_t> block_addresses;
    std::vector<std::string> blocks;

    IdToAddressMapping loadmap(char*, const uint64_t&);
    void calculateFunctionSizes();

    void loadMagicNumber(std::ifstream&);
    void assumeBinaryType(std::ifstream&, ViuaBinaryType);

    void loadMetaInformation(std::ifstream&);

    void loadExternalSignatures(std::ifstream&);
    void loadExternalBlockSignatures(std::ifstream&);
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

    std::vector<std::string> getExternalSignatures();
    std::vector<std::string> getExternalBlockSignatures();

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
