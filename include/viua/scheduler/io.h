/*
 *  Copyright (C) 2019 Marek Marecki
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

#ifndef VIUA_SCHEDULER_IO_H
#define VIUA_SCHEDULER_IO_H

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>


namespace viua {
namespace process {
class Process;
}
namespace kernel {
class Kernel;
}
}  // namespace viua

namespace viua::types {
    struct IO_interaction;
}


namespace viua { namespace scheduler { namespace io {
void io_scheduler(
    uint64_t const,
    viua::kernel::Kernel&,
    std::deque<std::unique_ptr<viua::types::IO_interaction>>& requests,
    std::mutex& mtx,
    std::condition_variable& cv);
}}}  // namespace viua::scheduler::io


#endif
