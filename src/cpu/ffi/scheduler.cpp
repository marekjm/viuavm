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

#include <vector>
#include <thread>
#include <condition_variable>
#include <viua/types/exception.h>
#include <viua/cpu/cpu.h>
using namespace std;


void ff_call_processor(vector<ForeignFunctionCallRequest*> *requests, map<string, ForeignFunction*>* foreign_functions, mutex *ff_map_mtx, mutex *mtx, condition_variable *cv) {
    while (true) {
        unique_lock<mutex> lock(*mtx);

        // wait in a loop, because wait_for() can still return even if the requests queue is empty
        while (not cv->wait_for(lock, chrono::milliseconds(2000), [requests](){
            return not requests->empty();
        }));

        ForeignFunctionCallRequest *request = requests->front();

        requests->erase(requests->begin());

        // unlock as soon as the request is obtained
        lock.unlock();

        // abort if received poison pill
        if (request == nullptr) {
            break;
        }

        string call_name = request->functionName();
        unique_lock<mutex> ff_map_lock(*ff_map_mtx);
        if (foreign_functions->count(call_name) == 0) {
            request->registerException(new Exception("call to unregistered foreign function: " + call_name));
        } else {
            auto function = foreign_functions->at(call_name);
            ff_map_lock.unlock();   // unlock the mutex - foreign call can block for unspecified period of time
            request->call(function);
        }

        request->wakeup();
        delete request;
    }
}
