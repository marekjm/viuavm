/*
 *  Copyright (C) 2018 Marek Marecki
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

#include <string>
#include <sys/stat.h>
#include <viua/util/filesystem.h>

namespace viua { namespace util { namespace filesystem {
auto is_file(std::string const& path) -> bool
{
    struct stat sf;

    // not a file if stat returned error
    if (stat(path.c_str(), &sf) == -1) {
        return false;
    }
    // not a file if S_ISREG() macro returned false
    if (not S_ISREG(sf.st_mode)) {
        return false;
    }

    // file otherwise
    return true;
}
}}}  // namespace viua::util::filesystem
