#ifndef SUPPORT_STRING_H
#define SUPPORT_STRING_H

#pragma once

#include <string>
#include <vector>
#include <sstream>


namespace str {
    bool startswith(const std::string& s, const std::string& w);
    bool startswithchunk(const std::string& s, const std::string& w);
    bool endswith(const std::string& s, const std::string& w);

    bool isnum(const std::string& s, bool negatives = true);
    bool isfloat(const std::string& s, bool negatives = true);

    std::string sub(const std::string& s, long unsigned b = 0, long int e = -1);

    std::string extract(const std::string& s);
    std::string chunk(const std::string& s, bool ignore_leading_ws = true);
    std::vector<std::string> chunks(const std::string& s);

    std::string join(const std::string& s, const std::vector<std::string>& v);
    template<typename T> std::string join(const std::vector<std::string>& seq, const T& delim) {
        unsigned sz = seq.size();
        std::ostringstream oss;
        for (unsigned i = 0; i < sz; ++i) {
            oss << seq[i];
            if (i < (sz-1)) {
                oss << delim;
            }
        }
        return oss.str();
    }

    template<typename T> std::string strmul(const T& s, unsigned times) {
        std::ostringstream oss;
        for (unsigned i = 0; i < times; ++i) {
            oss << s;
        }
        return oss.str();
    }

    std::string lstrip(const std::string& s);

    unsigned lshare(const std::string& s, const std::string& w);
    bool contains(const std::string&s, const char c);

    std::string enquote(const std::string&);
    std::string strdecode(const std::string&);

    std::string stringify(const std::vector<std::string>&);
    std::string stringify(unsigned);
}


#endif
