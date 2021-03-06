;
;   Copyright (C) 2015, 2016, 2017, 2018 Marek Marecki
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
    allocate_registers %3 local

    echo (string %1 local "Hello from #") local
    echo (move %2 local %0 parameters) local
    print (string %1 local " worker process!") local
    return
.end

.function: process_spawner/1
    allocate_registers %5 local

    echo (string %1 local "number of worker processes: ") local

    .name: 1 limit
    print (arg %limit local %0) local

    .name: 3 counter
    izero %counter local

    .mark: begin_loop
    if (gte %4 local %counter local %limit local) local end_loop +1

    frame ^[(copy %0 arguments %counter local)]
    process void worker_process/1

    iinc %counter local
    jump begin_loop
    .mark: end_loop

    return
.end


; std::io::getline/0 is required to get user input
.signature: std::io::getline/0

.function: get_number_of_processes_to_spawn/0
    allocate_registers %5 local

    echo (string %4 local "number of processes to spawn: ") local
    frame %0
    stoi %0 local (call %4 local std::io::getline/0) local
    return
.end

.function: run_process_spawner/1
    allocate_registers %2 local

    frame ^[(copy %0 arguments (move %1 local %0 parameters) local)]
    process void process_spawner/1
    return
.end

.function: main/1
    allocate_registers %2 local

    ; the standard "io" library contains std::io::getline
    import "io"

    frame %0
    call %1 local get_number_of_processes_to_spawn/0

    frame ^[(copy %0 arguments %1 local)]
    call run_process_spawner/1

    izero %0 local
    return
.end
