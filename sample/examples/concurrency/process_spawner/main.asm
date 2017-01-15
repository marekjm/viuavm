;
;   Copyright (C) 2015, 2016 Marek Marecki
;
;   This file is part of Viua VM.
;
;   Viua VM is free software: you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation, either version 3 of the License, or
;   (at your option) any later version.
;
;   Viua VM is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
;

.function: worker_process/1
    echo (strstore %1 "Hello from #")
    echo (arg %2 %0)
    print (strstore %1 " worker process!")
    return
.end

.function: process_spawner/1
    echo (strstore %1 "number of worker processes: ")

    .name: 1 limit
    print (arg %limit %0)

    .name: 3 counter
    izero %counter

    .mark: begin_loop
    if (gte int64 %4 %counter %limit) end_loop +1

    frame ^[(param %0 %counter)]
    process void worker_process/1

    iinc %counter
    jump begin_loop
    .mark: end_loop

    return
.end


; std::io::getline/0 is required to get user input
.signature: std::io::getline/0

.function: get_number_of_processes_to_spawn/0
    echo (strstore %4 "number of processes to spawn: ")
    frame %0
    stoi %0 (call %4 std::io::getline/0)
    return
.end

.function: run_process_spawner/1
    frame ^[(param %0 (arg %1 %0))]
    process void process_spawner/1
    return
.end

.function: main/1
    ; the standard "io" library contains std::io::getline
    import "io"

    frame %0
    call %4 get_number_of_processes_to_spawn/0

    frame ^[(param %0 %4)]
    call run_process_spawner/1

    izero %0
    return
.end
