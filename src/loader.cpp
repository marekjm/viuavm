#include <cstdint>
#include <iostream>
#include <fstream>
#include <tuple>
#include <string>
#include <vector>
#include <map>
#include "bytecode/bytetypedef.h"
#include "loader.h"
using namespace std;



IdToAddressMapping Loader::loadmap(char* bytedump, const uint16_t& bytedump_size) {
    vector<string> order;
    map<string, uint16_t> mapping;

    char *lib_function_ids_map = bytedump;

    int i = 0;
    string lib_fn_name;
    uint16_t lib_fn_address;
    while (i < bytedump_size) {
        lib_fn_name = string(lib_function_ids_map);
        i += lib_fn_name.size() + 1;  // one for null character
        lib_fn_address = *((uint16_t*)(bytedump+i));
        i += sizeof(uint16_t);
        lib_function_ids_map = bytedump+i;
        mapping[lib_fn_name] = lib_fn_address;
        order.push_back(lib_fn_name);
    }

    return IdToAddressMapping(order, mapping);
}
void Loader::calculateFunctionSizes() {
    string name;
    unsigned el_size;

    for (unsigned i = 0; i < functions.size(); ++i) {
        name = functions[i];
        el_size = 0;

        if (i < (functions.size()-1)) {
            long unsigned a = (unsigned long)(function_addresses[name]);
            long unsigned b = (unsigned long)(function_addresses[functions[i+1]]);
            el_size = (b-a);
        } else {
            long unsigned a = (long unsigned)(function_addresses[name]);
            long unsigned b = (long unsigned)size;
            el_size = (b-a);
        }

        function_sizes[name] = el_size;
    }
}

void Loader::loadJumpTable(ifstream& in) {
    // load jump table
    unsigned lib_total_jumps;
    in.read((char*)&lib_total_jumps, sizeof(unsigned));

    unsigned lib_jmp;
    for (unsigned i = 0; i < lib_total_jumps; ++i) {
        in.read((char*)&lib_jmp, sizeof(unsigned));
        jumps.push_back(lib_jmp);
    }
}
void Loader::loadFunctionsMap(ifstream& in) {
    uint16_t lib_function_ids_section_size = 0;
    in.read((char*)&lib_function_ids_section_size, sizeof(uint16_t));

    char *lib_buffer_function_ids = new char[lib_function_ids_section_size];
    in.read(lib_buffer_function_ids, lib_function_ids_section_size);

    vector<string> order;
    map<string, uint16_t> mapping;

    tie(order, mapping) = loadmap(lib_buffer_function_ids, lib_function_ids_section_size);

    for (string p : order) {
        functions.push_back(p);
        function_addresses[p] = mapping[p];
    }
    delete[] lib_buffer_function_ids;
}
void Loader::loadBlocksMap(ifstream& in) {
    uint16_t lib_block_ids_section_size = 0;
    in.read((char*)&lib_block_ids_section_size, sizeof(uint16_t));

    char *lib_buffer_block_ids = new char[lib_block_ids_section_size];
    in.read(lib_buffer_block_ids, lib_block_ids_section_size);

    vector<string> order;
    map<string, uint16_t> mapping;

    tie(order, mapping) = loadmap(lib_buffer_block_ids, lib_block_ids_section_size);

    for (string p : order) {
        blocks.push_back(p);
        block_addresses[p] = mapping[p];
    }
    delete[] lib_buffer_block_ids;
}
void Loader::loadBytecode(ifstream& in) {
    in.read((char*)&size, 16);
    bytecode = new byte[size];
    in.read((char*)bytecode, size);
}

Loader& Loader::load() {
    ifstream in(path, ios::in | ios::binary);
    if (!in) {
        throw ("fatal: failed to link " + path);
    }

    // jump table must be loaded if loading a library
    loadJumpTable(in);

    loadBlocksMap(in);
    loadFunctionsMap(in);
    loadBytecode(in);
    calculateFunctionSizes();

    return (*this);
}

Loader& Loader::executable() {
    ifstream in(path, ios::in | ios::binary);
    if (!in) {
        throw ("fatal: failed to link " + path);
    }

    loadBlocksMap(in);
    loadFunctionsMap(in);
    loadBytecode(in);
    calculateFunctionSizes();

    return (*this);
}

uint16_t Loader::getBytecodeSize() {
    return size;
}
byte* Loader::getBytecode() {
    byte* copy = new byte[size];
    for (unsigned i = 0; i < size; ++i) {
        copy[i] = bytecode[i];
    }
    return copy;
}

vector<unsigned> Loader::getJumps() {
    return jumps;
}

map<string, uint16_t> Loader::getFunctionAddresses() {
    return function_addresses;
}
map<string, unsigned> Loader::getFunctionSizes() {
    return function_sizes;
}
vector<string> Loader::getFunctions() {
    return functions;
}

map<string, uint16_t> Loader::getBlockAddresses() {
    return block_addresses;
}
vector<string> Loader::getBlocks() {
    return blocks;
}
