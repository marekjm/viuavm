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

#include <condition_variable>
#include <memory>
#include <thread>
#include <vector>
#include <viua/kernel/frame.h>
#include <viua/scheduler/ffi.h>
#include <viua/types/exception.h>


void viua::scheduler::ffi::ff_call_processor(
    std::vector<std::unique_ptr<
        viua::scheduler::ffi::Foreign_function_call_request>>* requests,
    std::map<std::string, ForeignFunction*>* foreign_functions,
    std::mutex* ff_map_mtx,
    std::mutex* mtx,
    std::condition_variable* cv) {
    while (true) {
        std::unique_lock<std::mutex> lock(*mtx);

        // wait in a loop, because wait_for() can still return even if the
        // requests queue is empty
        while (not cv->wait_for(lock,
                                std::chrono::milliseconds(2000),
                                [requests]() { return not requests->empty(); }))
            ;

        std::unique_ptr<Foreign_function_call_request> request(
            std::move(requests->front()));
        requests->erase(requests->begin());

        // unlock as soon as the request is obtained
        lock.unlock();

        // abort if received poison pill
        if (request == nullptr) {
            break;
        }

        std::string call_name = request->function_name();
        std::unique_lock<std::mutex> ff_map_lock(*ff_map_mtx);
        if (foreign_functions->count(call_name) == 0) {
            request->raise(std::make_unique<viua::types::Exception>(
                "call to unregistered foreign function: " + call_name));
        } else {
            auto function = foreign_functions->at(call_name);
            ff_map_lock.unlock();  // unlock the mutex - foreign call can block
                                   // for unspecified period of time
            request->call(function);
        }

        request->wakeup();
    }
}
