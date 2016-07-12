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

#ifndef SUPPORT_ENV_H
#define SUPPORT_ENV_H

#pragma once

#include <sys/stat.h>
#include <string>
#include <vector>

namespace support {
    namespace env {
        std::vector<std::string> getpaths(const std::string&);
        std::string getvar(const std::string&);

        bool isfile(const std::string&);

        namespace viua {
            std::string getmodpath(const std::string&, const std::string&, const std::vector<std::string>&);
        }
    }
}

#endif
