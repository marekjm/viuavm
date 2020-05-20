/*
 *  Copyright (C) 2016 Marek Marecki
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

#ifndef VIUA_SCHEDULER_FFI_H
#define VIUA_SCHEDULER_FFI_H

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <viua/include/module.h>


namespace viua {
namespace process {
class Process;
}
namespace kernel {
class Kernel;
}
}  // namespace viua


namespace viua { namespace scheduler { namespace ffi {
class Foreign_function_call_request {
    std::unique_ptr<Frame> frame;
    viua::process::Process& caller_process;
    viua::kernel::Kernel& kernel;

  public:
    auto function_name() const -> std::string;
    auto call(ForeignFunction*) -> void;
    auto raise(std::unique_ptr<viua::types::Value>) -> void;
    auto wakeup() -> void;

    Foreign_function_call_request(Frame* fr,
                                  viua::process::Process* cp,
                                  viua::kernel::Kernel* c)
            : frame(fr), caller_process(*cp), kernel{*c}
    {}
    Foreign_function_call_request(std::unique_ptr<Frame> fr,
                                  viua::process::Process& cp,
                                  viua::kernel::Kernel& c)
            : frame{std::move(fr)}, caller_process{cp}, kernel{c}
    {}
    ~Foreign_function_call_request() {}
};

void ff_call_processor(
    std::vector<std::unique_ptr<Foreign_function_call_request>>* requests,
    std::map<std::string, ForeignFunction*>* foreign_functions,
    std::mutex* ff_map_mtx,
    std::mutex* mtx,
    std::condition_variable* cv);
}}}  // namespace viua::scheduler::ffi


#endif
