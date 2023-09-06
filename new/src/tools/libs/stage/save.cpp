/*
 *  Copyright (C) 2023 Marek Marecki
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

#include <elf.h>
#include <endian.h>
#include <stdint.h>
#include <string.h>

#include <algorithm>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <viua/libs/stage.h>


namespace viua::libs::stage {
auto save_string_to_strtab(std::vector<uint8_t>& tab,
                           std::string_view const data) -> size_t
{
    {
        /*
         * Scan the strings table to see if the requested data is already there.
         * There is no reason to store extra copies of the same value in .rodata
         * so let's deduplicate.
         *
         * FIXME This is ridiculously slow. Maybe add some cache instead of
         * linearly browsing through the whole table every time?
         */
        auto i = size_t{0};
        while ((i + data.size()) < tab.size()) {
            auto const existing = std::string_view{
                reinterpret_cast<char const*>(tab.data() + i), data.size()};

            auto const content_matches = (existing == data);
            auto const nul_matches = (*(tab.data() + i + data.size()) == '\0');
            if (content_matches and nul_matches) {
                return i;
            }

            ++i;
        }
    }

    auto const saved_location = tab.size();

    std::copy(data.begin(), data.end(), std::back_inserter(tab));
    tab.push_back('\0');

    return saved_location;
}
auto save_buffer_to_rodata(std::vector<uint8_t>& strings,
                           std::string_view const data) -> size_t
{
    {
        /*
         * Scan the strings table to see if the requested data is already there.
         * There is no reason to store extra copies of the same value in .rodata
         * so let's deduplicate.
         *
         * FIXME This is ridiculously slow. Maybe add some cache instead of
         * linearly browsing through the whole table every time?
         */
        auto i = size_t{0};
        while (i < strings.size()) {
            auto data_size = uint64_t{};
            memcpy(&data_size, strings.data() + i, sizeof(data_size));
            data_size = le64toh(data_size);

            i += sizeof(data_size);

            auto const existing = std::string_view{
                reinterpret_cast<char const*>(strings.data() + i), data_size};
            if (existing == data) {
                return i;
            }

            i += data_size;
        }
    }

    auto const data_size = htole64(static_cast<uint64_t>(data.size()));
    strings.resize(strings.size() + sizeof(data_size));
    memcpy((strings.data() + strings.size() - sizeof(data_size)),
           &data_size,
           sizeof(data_size));

    auto const saved_location = strings.size();
    std::copy(data.begin(), data.end(), std::back_inserter(strings));

    return saved_location;
}

auto record_symbol(std::string name,
                   Elf64_Sym const symbol,
                   std::vector<Elf64_Sym>& symbol_table,
                   std::map<std::string, size_t>& symbol_map) -> size_t
{
    auto const sym_ndx          = symbol_table.size();
    symbol_map[std::move(name)] = sym_ndx;
    symbol_table.push_back(symbol);
    return sym_ndx;
}
}  // namespace viua::libs::stage
