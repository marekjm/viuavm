#ifndef SUPPORT_STRING_H
#define SUPPORT_STRING_H

#pragma once

#include <string>


namespace str {
    bool startswith(const std::string& s, const std::string& w);
    bool startswithchunk(const std::string& s, const std::string& w);
    bool endswith(const std::string& s, const std::string& w);

    bool isnum(const std::string& s);

    std::string sub(const std::string& s, int b = 0, int e = -1);
    std::string chunk(const std::string& s, bool ignore_leading_ws=true);

    std::string lstrip(const std::string& s);

    unsigned lshare(const std::string& s, const std::string& w);

    std::string enquote(const std::string&);
}


#endif
