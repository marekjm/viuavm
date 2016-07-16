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

#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>
#include <tuple>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <viua/machine.h>
#include <viua/bytecode/bytetypedef.h>
#include <viua/loader.h>
using namespace std;


IdToAddressMapping Loader::loadmap(char* bytedump, const uint64_t& bytedump_size) {
    vector<string> order;
    map<string, uint64_t> mapping;

    char *lib_function_ids_map = bytedump;

    long unsigned i = 0;
    string lib_fn_name;
    uint64_t lib_fn_address;
    while (i < bytedump_size) {
        lib_fn_name = string(lib_function_ids_map);
        i += lib_fn_name.size() + 1;  // one for null character
        lib_fn_address = *reinterpret_cast<decltype(lib_fn_address)*>(bytedump+i);
        i += sizeof(decltype(lib_fn_address));
        lib_function_ids_map = bytedump+i;
        mapping[lib_fn_name] = lib_fn_address;
        order.push_back(lib_fn_name);
    }

    return IdToAddressMapping(order, mapping);
}
void Loader::calculateFunctionSizes() {
    string name;
    uint64_t el_size;

    for (unsigned i = 0; i < functions.size(); ++i) {
        name = functions[i];
        el_size = 0;

        if (i < (functions.size()-1)) {
            uint64_t a = function_addresses[name];
            uint64_t b = function_addresses[functions[i+1]];
            el_size = (b-a);
        } else {
            uint64_t a = function_addresses[name];
            uint64_t b = size;
            el_size = (b-a);
        }

        function_sizes[name] = el_size;
    }
}

void Loader::loadMagicNumber(ifstream& in) {
    char magic_number[5];
    in.read(magic_number, sizeof(char)*5);
    if (magic_number[4] != '\0') {
        throw "invalid magic number";
    }
    if (string(magic_number) != string(VIUA_MAGIC_NUMBER)) {
        throw (string("invalid magic number: ") + string(magic_number));
    }
}

void Loader::assumeBinaryType(ifstream& in, ViuaBinaryType assumed_binary_type) {
    char bt;
    in.read(&bt, sizeof(decltype(bt)));
    if (bt != assumed_binary_type) {
        ostringstream error;
        error << "not a " << (assumed_binary_type == VIUA_LINKABLE ? "linkable" : "executable") << " file: " << path;
        throw error.str();
    }
}

map<string, string> load_meta_information_map(ifstream& in) {
    uint64_t meta_information_map_size = 0;
    readinto(in, &meta_information_map_size);

    unique_ptr<char[]> meta_information_map_buffer(new char[meta_information_map_size]);
    in.read(meta_information_map_buffer.get(), meta_information_map_size);

    map<string, string> meta_information_map;

    char *buffer = meta_information_map_buffer.get();
    uint64_t i = 0;
    string key, value;
    while (i < meta_information_map_size) {
        key = string(buffer+i);
        i += (key.size() + 1);
        value = string(buffer+i);
        i += (value.size() + 1);
        meta_information_map[key] = value;
    }

    return meta_information_map;
}
void Loader::loadMetaInformation(ifstream& in) {
    meta_information = std::move(load_meta_information_map(in));
}

vector<string> load_string_list(ifstream& in) {
    uint64_t signatures_section_size = 0;
    readinto(in, &signatures_section_size);

    unique_ptr<char[]> signatures_section_buffer(new char[signatures_section_size]);
    in.read(signatures_section_buffer.get(), signatures_section_size);

    uint64_t i = 0;
    char *buffer = signatures_section_buffer.get();
    string sig;
    vector<string> strings_list;
    while (i < signatures_section_size) {
        sig = string(buffer+i);
        i += (sig.size() + 1);
        strings_list.push_back(sig);
    }

    return strings_list;
}
void Loader::loadExternalSignatures(ifstream& in) {
    external_signatures = std::move(load_string_list(in));
}
void Loader::loadExternalBlockSignatures(ifstream& in) {
    external_signatures_block = std::move(load_string_list(in));
}

void Loader::loadJumpTable(ifstream& in) {
    // load jump table
    uint64_t lib_total_jumps;
    readinto(in, &lib_total_jumps);

    uint64_t lib_jmp;
    for (uint64_t i = 0; i < lib_total_jumps; ++i) {
        readinto(in, &lib_jmp);
        jumps.push_back(lib_jmp);
    }
}
void Loader::loadFunctionsMap(ifstream& in) {
    uint64_t lib_function_ids_section_size = 0;
    readinto(in, &lib_function_ids_section_size);

    char *lib_buffer_function_ids = new char[lib_function_ids_section_size];
    in.read(lib_buffer_function_ids, static_cast<std::streamsize>(lib_function_ids_section_size));

    vector<string> order;
    map<string, uint64_t> mapping;

    tie(order, mapping) = loadmap(lib_buffer_function_ids, lib_function_ids_section_size);

    for (string p : order) {
        functions.push_back(p);
        function_addresses[p] = mapping[p];
    }
    delete[] lib_buffer_function_ids;
}
void Loader::loadBlocksMap(ifstream& in) {
    uint64_t lib_block_ids_section_size = 0;
    readinto(in, &lib_block_ids_section_size);

    char *lib_buffer_block_ids = new char[lib_block_ids_section_size];
    in.read(lib_buffer_block_ids, static_cast<std::streamsize>(lib_block_ids_section_size));

    vector<string> order;
    map<string, uint64_t> mapping;

    tie(order, mapping) = loadmap(lib_buffer_block_ids, lib_block_ids_section_size);

    for (string p : order) {
        blocks.push_back(p);
        block_addresses[p] = mapping[p];
    }
    delete[] lib_buffer_block_ids;
}
void Loader::loadBytecode(ifstream& in) {
    in.read(reinterpret_cast<char*>(&size), sizeof(decltype(size)));
    bytecode = new byte[size];
    in.read(reinterpret_cast<char*>(bytecode), static_cast<std::streamsize>(size));
}

Loader& Loader::load() {
    ifstream in(path, ios::in | ios::binary);
    if (!in) {
        throw ("failed to open file: " + path);
    }

    loadMagicNumber(in);
    assumeBinaryType(in, VIUA_LINKABLE);

    loadMetaInformation(in);

    // jump table must be loaded if loading a library
    loadJumpTable(in);

    loadExternalSignatures(in);
    loadExternalBlockSignatures(in);
    loadBlocksMap(in);
    loadFunctionsMap(in);
    loadBytecode(in);
    calculateFunctionSizes();

    return (*this);
}

Loader& Loader::executable() {
    ifstream in(path, ios::in | ios::binary);
    if (!in) {
        throw ("fatal: failed to open file: " + path);
    }

    loadMagicNumber(in);
    assumeBinaryType(in, VIUA_EXECUTABLE);

    loadMetaInformation(in);

    loadExternalSignatures(in);
    loadExternalBlockSignatures(in);
    loadBlocksMap(in);
    loadFunctionsMap(in);
    loadBytecode(in);
    calculateFunctionSizes();

    return (*this);
}

uint64_t Loader::getBytecodeSize() {
    return size;
}
byte* Loader::getBytecode() {
    byte* copy = new byte[size];
    for (uint64_t i = 0; i < size; ++i) {
        copy[i] = bytecode[i];
    }
    return copy;
}

vector<uint64_t> Loader::getJumps() {
    return jumps;
}

map<string, string> Loader::getMetaInformation() {
    return meta_information;
}

vector<string> Loader::getExternalSignatures() {
    return external_signatures;
}

vector<string> Loader::getExternalBlockSignatures() {
    return external_signatures_block;
}

map<string, uint64_t> Loader::getFunctionAddresses() {
    return function_addresses;
}
map<string, uint64_t> Loader::getFunctionSizes() {
    return function_sizes;
}
vector<string> Loader::getFunctions() {
    return functions;
}

map<string, uint64_t> Loader::getBlockAddresses() {
    return block_addresses;
}
vector<string> Loader::getBlocks() {
    return blocks;
}
