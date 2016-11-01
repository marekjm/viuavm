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

#include <memory>
#include <mutex>
#include <condition_variable>
#include <viua/include/module.h>


class Kernel;
class Process;


namespace viua {
    namespace scheduler {
        namespace ffi {
            class ForeignFunctionCallRequest {
                std::unique_ptr<Frame> frame;
                Process *caller_process;
                Kernel *kernel;

                public:
                    std::string functionName() const;
                    void call(ForeignFunction*);
                    void registerException(viua::types::Type*);
                    void wakeup();

                    ForeignFunctionCallRequest(Frame *fr, Process *cp, Kernel *c): frame(fr), caller_process(cp), kernel(c) {}
                    ~ForeignFunctionCallRequest() {}
            };

            void ff_call_processor(std::vector<std::unique_ptr<ForeignFunctionCallRequest>> *requests, std::map<std::string, ForeignFunction*> *foreign_functions, std::mutex *ff_map_mtx, std::mutex *mtx, std::condition_variable *cv);
        }
    }
}


#endif
