#ifndef SUPPORT_STRING_H
#define SUPPORT_STRING_H

#pragma once

#include <string>


namespace str {
    bool startswith(const std::string& s, const std::string& w);

    bool isnum(const std::string& s);

    std::string chunk(const std::string& s);
};


#endif
