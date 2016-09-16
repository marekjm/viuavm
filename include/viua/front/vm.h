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

#include <string>
#include <vector>
#include <viua/support/string.h>
#include <viua/support/env.h>
#include <viua/types/pointer.h>
#include <viua/types/exception.h>
#include <viua/types/string.h>
#include <viua/types/process.h>
#include <viua/loader.h>
#include <viua/kernel/kernel.h>


namespace viua {
    namespace front {
        namespace vm {
            void initialise(Kernel*, const std::string&, std::vector<std::string>);
            void load_standard_prototypes(Kernel*);
            void preload_libraries(Kernel*);
        }
    }
}
