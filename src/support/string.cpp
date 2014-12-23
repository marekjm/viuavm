#include <string>
#include "string.h"
using namespace std;


namespace str {
    bool startswith(const std::string& s, const std::string& w) {
        return (s.compare(0, w.length(), w) == 0);
    }

    bool isnum(const std::string& s) {
        return false;
    }

    std::string chunk(const std::string& s) {
        return "";
    }
};
