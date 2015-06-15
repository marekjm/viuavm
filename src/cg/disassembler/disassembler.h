#ifndef VIUA_DISASSEMBLER_H
#define VIUA_DISASSEMBLER_H

#pragma once

#include <algorithm>
#include <string>
#include <tuple>


// Helper functions for checking if a container contains an item.
template<typename T> bool in(std::vector<T> v, T item) {
    return (std::find(v.begin(), v.end(), item) != v.end());
}
template<typename K, typename V> bool in(std::map<K, V> m, K key) {
    return (m.count() == 1);
}

namespace disassembler {
    std::tuple<std::string, unsigned> instruction(byte*);
}


#endif
