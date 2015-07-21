
#ifndef SUPPORT_ENV_H
#define SUPPORT_ENV_H

#pragma once

#include <sys/stat.h>
#include <string>
#include <vector>

namespace support {
    namespace env {
        std::vector<std::string> getpaths(const std::string&);

        bool isfile(const std::string&);

        namespace viua {
            std::string getmodpath(const std::string&, const std::vector<std::string>&);
        }
    }
}

#endif
