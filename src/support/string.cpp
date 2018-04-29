/*
 *  Copyright (C) 2015, 2016, 2018 Marek Marecki
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

#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <viua/types/string.h>


namespace str {
auto startswith(std::string const& s, std::string const& w) -> bool {
    /*  Returns true if s stars with w.
     */
    return (s.compare(0, w.length(), w) == 0);
}

auto startswithchunk(std::string const& s, std::string const& w) -> bool {
    /*  Returns true if s stars with chunk w.
     */
    return (chunk(s) == w);
}

auto endswith(std::string const& s, std::string const& w) -> bool {
    /*  Returns true if s ends with w.
     */
    return (s.compare(s.length() - w.length(), s.length(), w) == 0);
}


auto isnum(std::string const& s, bool negatives) -> bool {
    /*  Returns true if s contains only numerical characters.
     *  Regex equivalent: `^[0-9]+$`
     */
    auto num   = bool{false};
    auto start = std::string::size_type{0};
    if (s[0] == '-' and negatives) {
        // must handle negative numbers
        start = 1;
    }
    for (auto i = start; i < s.size(); ++i) {
        switch (s[i]) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            num = true;
            break;
        default:
            num = false;
        }
        if (!num)
            break;
    }
    return num;
}

auto ishex(std::string const& s, bool) -> bool {
    /*  Returns true if s is a valid hexadecimal number.
     */
    static auto const hexadecimal_number = std::regex{"^0x[0-9a-fA-F]+$"};
    return regex_match(s, hexadecimal_number);
}

auto is_binary_literal(std::string const s) -> bool {
    static auto const binary_literal =
        std::regex{"^0(?:b[01]+|o[0-7]+|x[0-9a-f]+)$"};
    return regex_match(s, binary_literal);
}

auto is_boolean_literal(std::string const s) -> bool {
    return (s == "true" or s == "false");
}

auto is_void(std::string const s) -> bool {
    return (s == "void");
}

auto is_atom_literal(std::string const s) -> bool {
    /*
     * This seemingly naive check is sufficient, as this function should only
     * be called after the source code already lexed (and the lexer ensures that
     * strings are properly closed and escaped so it is sufficient to check
     * for opening and closing quotes here).
     */
    return (s.at(0) == '\'' and s.at(s.size() - 1) == '\'');
}

auto is_text_literal(std::string const s) -> bool {
    /*
     * Same as with with is_atom_literal().
     */
    return (s.at(0) == '"' and s.at(s.size() - 1) == '"');
}

auto is_timeout_literal(std::string const s) -> bool {
    if (s == "infinity") {
        return true;
    }

    const auto size = s.size();
    if (size < 2) {
        return false;
    }
    if (s.at(size - 2) == 'm' and s.at(size - 1) == 's'
        and str::isnum(s.substr(0, size - 2))) {
        return true;
    }
    if (s.at(size - 1) == 's' and str::isnum(s.substr(0, size - 1))) {
        return true;
    }
    return false;
}

auto is_register_set_name(std::string const s) -> bool {
    return (s == "local" or s == "static" or s == "global" or s == "current");
}

auto isfloat(std::string const& s, bool negatives) -> bool {
    /*  Returns true if s contains only numerical characters.
     *  Regex equivalent: `^[0-9]+\.[0-9]+$`
     */
    auto is    = bool{false};
    auto start = std::string::size_type{0};
    if (s[0] == '-' and negatives) {
        // to handle negative numbers
        start = 1;
    }
    auto dot = int{-1};
    for (auto i = start; i < s.size(); ++i) {
        if (s[i] == '.') {
            dot = static_cast<int>(i);
            break;
        }
    }
    if (dot == -1) {
        return false;
    }
    is = isnum(sub(s, 0, dot), negatives)
         and isnum(sub(s, (static_cast<unsigned>(dot) + 1)));
    return is;
}

auto isid(std::string const& s) -> bool {
    /*  Returns true if s is a valid identifier.
     */
    static auto const identifier = std::regex{"^[a-zA-Z_][:/a-zA-Z0-9_]*$"};
    return regex_match(s, identifier);
}


auto sub(std::string const& s, std::string::size_type b, long int e)
    -> std::string {
    /*  Returns substring of s.
     *  If only s is passed, returns copy of s.
     */
    if (b == 0 and e == -1) {
        return std::string(s);
    }

    auto part = std::ostringstream{};
    part.str("");

    auto end = std::string::size_type{0};
    if (e < 0) {
        end = (s.size() - static_cast<std::string::size_type>(-1 * e) + 1);
    } else {
        end = static_cast<std::string::size_type>(e);
    }

    for (auto i = b; i < s.size() and i < end; ++i) {
        part << s[i];
    }

    return part.str();
}


auto chunk(std::string const& s, bool ignore_leading_ws) -> std::string {
    /*  Returns part of the std::string until first whitespace from left side.
     */
    auto chnk = std::ostringstream{};

    auto str = std::string{ignore_leading_ws ? lstrip(s) : s};

    for (auto i = std::string::size_type{0}; i < str.size(); ++i) {
        if (str[i] == ' ' or str[i] == '\t' or str[i] == '\v'
            or str[i] == '\n') {
            break;
        }
        chnk << str[i];
    }
    return chnk.str();
}

auto chunks(std::string const& s) -> std::vector<std::string> {
    /*  Returns chunks of std::string.
     */
    auto chnks = std::vector<std::string>{};
    auto tmp   = lstrip(s);
    auto chnk  = std::string{};
    while (tmp.size()) {
        chnk = chunk(tmp);
        tmp  = lstrip(sub(tmp, chnk.size()));
        chnks.emplace_back(chnk);
    }
    return chnks;
}


auto join(std::string const& s, std::vector<std::string> const& parts)
    -> std::string {
    /** Join elements of vector with given std::string.
     */
    auto oss         = std::ostringstream{};
    auto const limit = parts.size();
    for (auto i = std::remove_reference_t<decltype(parts)>::size_type{0};
         i < limit;
         ++i) {
        oss << parts[i];
        if (i < (limit - 1)) {
            oss << s;
        }
    }
    return oss.str();
}


auto extract(std::string const& s) -> std::string {
    /** Extracts *enquoted chunk*.
     *
     *  It is particularly useful if you have a std::string encoded in another
     * std::string.
     *
     *  This function will return `"Hello 'Beautiful' World!"` when fed `"Hello
     * 'Beautiful' World!" some other (42) things;`, and will return `'Hello
     * "Beautiful" World!'` when fed `'Hello "Beautiful" World!' some other (42)
     * things;`.
     *  Starting quote character is irrelevant.
     *
     *  In fact, this function will treat *the first character* of the
     * std::string it is fed as a delimiter for std::string extraction -
     * whatever that may be (e.g. the backtick character) so you can get
     * creative. One character that is not recommended for use as a delimiter is
     * the backslash as it is treated specially (as the escape character) by
     * this function.
     */
    if (s.size() == 0) {
        return std::string("");
    }

    auto chnk  = std::ostringstream{};
    auto quote = char{};
    chnk << (quote = s[0]);

    auto backs = std::string::size_type{0};
    for (auto i = std::string::size_type{1}; i < s.size(); ++i) {
        chnk << s[i];
        if (backs and s[i] != '\\' and s[i] != quote) {
            backs = 0;
            continue;
        }
        if (s[i] == quote and ((backs % 2) != 0)) {
            backs = 0;
            continue;
        } else if (s[i] == quote and ((backs % 2) == 0)) {
            break;
        }
        if (s[i] == '\\') {
            ++backs;
            // cout << "backs = " << backs << endl;
        }
    }

    return chnk.str();
}


auto lstrip(std::string const& s) -> std::string {
    /*  Removes whitespace from left side of the std::string.
     */
    auto i = std::string::size_type{0};
    while (i < s.size()) {
        if (not(s[i] == ' ' or s[i] == '\t' or s[i] == '\v' or s[i] == '\n')) {
            break;
        };
        ++i;
    }
    return sub(s, i);
}


auto lshare(std::string const& s, std::string const& w)
    -> std::string::size_type {
    auto share = std::string::size_type{0};
    for (auto i = std::string::size_type{0}; i < s.size() and i < w.size();
         ++i) {
        if (s[i] == w[i]) {
            ++share;
        } else {
            break;
        }
    }
    return share;
}
auto contains(std::string const& s, char const c) -> bool {
    auto it_does = bool{false};
    for (auto i = std::string::size_type{0}; i < s.size(); ++i) {
        if (s[i] == c) {
            it_does = true;
            break;
        }
    }
    return it_does;
}


auto levenshtein(std::string const source, std::string const target)
    -> LevenshteinDistance {
    if (not source.size()) {
        return target.size();
    }
    if (not target.size()) {
        return source.size();
    }

    auto distance_matrix = std::vector<std::vector<LevenshteinDistance>>{};

    distance_matrix.reserve(source.size());
    for (auto i = LevenshteinDistance{0}; i < source.size() + 1; ++i) {
        decltype(distance_matrix)::value_type row;
        row.reserve(target.size());
        for (auto j = LevenshteinDistance{0}; j < target.size() + 1; ++j) {
            row.push_back(0);
        }
        distance_matrix.push_back(std::move(row));
    }
    for (auto i = LevenshteinDistance{0}; i < source.size() + 1; ++i) {
        distance_matrix.at(i).at(0) = i;
    }
    for (auto i = LevenshteinDistance{0}; i < target.size() + 1; ++i) {
        distance_matrix.at(0).at(i) = i;
    }

    for (auto i = LevenshteinDistance{1}; i < source.size() + 1; ++i) {
        for (auto j = LevenshteinDistance{1}; j < target.size() + 1; ++j) {
            auto cost = LevenshteinDistance{0};

            cost = (source.at(i - 1) != target.at(j - 1));

            auto deletion     = distance_matrix.at(i - 1).at(j) + 1;
            auto insertion    = distance_matrix.at(i).at(j - 1) + 1;
            auto substitution = distance_matrix.at(i - 1).at(j - 1) + cost;

            distance_matrix.at(i).at(j) =
                std::min(std::min(deletion, insertion), substitution);
        }
    }

    return distance_matrix.at(source.size() - 1).at(target.size() - 1);
}
auto levenshtein_filter(std::string const source,
                        std::vector<std::string> const& candidates,
                        LevenshteinDistance const limit)
    -> std::vector<DistancePair> {
    auto matched = std::vector<DistancePair>{};

    for (auto const each : candidates) {
        if (auto distance = levenshtein(source, each); distance <= limit) {
            matched.emplace_back(distance, each);
        }
    }

    return matched;
}
auto levenshtein_best(std::string const source,
                      std::vector<std::string> const& candidates,
                      LevenshteinDistance const limit) -> DistancePair {
    auto best = DistancePair{0, source};

    for (const auto each : levenshtein_filter(source, candidates, limit)) {
        if ((not best.first) or each.first < best.first) {
            best = each;
            continue;
        }
    }

    return best;
}


auto enquote(std::string const& s, char const closing) -> std::string {
    /** Enquote the std::string.
     */
    auto encoded = std::ostringstream{};

    encoded << closing;
    for (auto i = std::string::size_type{0}; i < s.size(); ++i) {
        if (s[i] == closing) {
            encoded << "\\";
        }
        encoded << s[i];
    }
    encoded << closing;

    return encoded.str();
}

auto strdecode(std::string const& s) -> std::string {
    /** Decode escape sequences in strings.
     *
     *  This function recognizes escape sequences as listed on:
     *  http://en.cppreference.com/w/cpp/language/escape
     *  The function does not recognize sequences for:
     *      - arbitrary octal numbers (escape: \nnn),
     *      - arbitrary hexadecimal numbers (escape: \xnn),
     *      - short arbitrary Unicode values (escape: \unnnn),
     *      - long arbitrary Unicode values (escape: \Unnnnnnnn),
     *
     *  If a character that does not encode an escape sequence is
     *  preceded by a backslash (\\) the function consumes the backslash and
     *  leaves only the character preceded by it in the output std::string.
     *
     */
    auto decoded = std::ostringstream{};
    auto c       = char{};
    for (auto i = std::string::size_type{0}; i < s.size(); ++i) {
        c = s[i];
        if (c == '\\' and i < (s.size() - 1)) {
            ++i;
            switch (s[i]) {
            case '\'':
                c = '\'';
                break;
            case '"':
                c = '"';
                break;
            case '?':
                c = '?';
                break;
            case '\\':
                c = '\\';
                break;
            case 'a':
                c = '\a';
                break;
            case 'b':
                c = '\b';
                break;
            case 'f':
                c = '\f';
                break;
            case 'n':
                c = '\n';
                break;
            case 'r':
                c = '\r';
                break;
            case 't':
                c = '\t';
                break;
            case 'v':
                c = '\v';
                break;
            default:
                c = s[i];
            }
        }
        decoded << c;
    }
    return decoded.str();
}
auto strencode(std::string const& s) -> std::string {
    /** Encode escape sequences in strings.
     *
     *  This function recognizes escape sequences as listed on:
     *  http://en.cppreference.com/w/cpp/language/escape
     *  The function does not recognize sequences for:
     *      - arbitrary octal numbers (escape: \nnn),
     *      - arbitrary hexadecimal numbers (escape: \xnn),
     *      - short arbitrary Unicode values (escape: \unnnn),
     *      - long arbitrary Unicode values (escape: \Unnnnnnnn),
     *
     */
    auto encoded = std::ostringstream{};
    auto c       = char{};
    auto escape  = bool{false};
    for (auto i = std::string::size_type{0}; i < s.size(); ++i) {
        switch (s[i]) {
        case '\\':
            escape = true;
            c      = '\\';
            break;
        case '\a':
            escape = true;
            c      = 'a';
            break;
        case '\b':
            escape = true;
            c      = 'b';
            break;
        case '\f':
            escape = true;
            c      = 'f';
            break;
        case '\n':
            escape = true;
            c      = 'n';
            break;
        case '\r':
            escape = true;
            c      = 'r';
            break;
        case '\t':
            escape = true;
            c      = 't';
            break;
        case '\v':
            escape = true;
            c      = 'v';
            break;
        default:
            escape = false;
            c      = s[i];
        }
        if (escape) {
            encoded << '\\';
        }
        encoded << c;
    }
    return encoded.str();
}


auto stringify(std::vector<std::string> const& sv) -> std::string {
    auto oss = std::ostringstream{};
    oss << '[';
    auto const sz = sv.size();
    for (std::remove_const_t<decltype(sz)> i = 0; i < sz; ++i) {
        oss << enquote(sv[i]);
        if (i < (sz - 1)) {
            oss << ", ";
        }
    }
    oss << ']';
    return oss.str();
}
}  // namespace str
