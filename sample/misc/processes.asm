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

.function: run_in_a_process/1
    allocate_registers %5 local

    move %1 local %0 parameters

    text %2 local "spawned process "
    text %3 local %1 local
    textconcat %2 local %2 local %3 local

    print %2 local

    integer %4 local 4000
    .mark: loop
    if (not %2 local %4 local) local done +1
    idec %4 local
    jump loop

    .mark: done

    text %2 local "exiting process "
    textconcat %2 local %2 local %3 local
    print %2 local
    return
.end

.function: spawn_processes/2
    allocate_registers %4 local

    .name: 1 counter
    move %counter local %0 parameters

    .name: 2 limit
    move %limit local %1 parameters

    ; if limit is N, processes IDs go from 0 to N-1
    ; this is why the counter is incremented before the
    ; branch instruction which checks for equality
    iinc %counter local

    ; if the counter is equal to limit return immediately
    if (eq %3 local %counter local %limit local) local +1 +2
    return

    ; spawn a new process
    frame ^[(copy %0 arguments %counter local)]
    process void run_in_a_process/1

    ; take advantage of tail recursion in Viua and
    ; elegantly follow with process spawner execution
    frame ^[(move %0 arguments %counter local) (move %1 arguments %limit local)]
    tailcall spawn_processes/2
.end

.function: process_spawner/1
    allocate_registers %4 local

    echo (string %2 local "process_spawner/1: ") local
    .name: 1 limit
    echo (move %1 local %0 parameters) local
    print (text %2 local " processs to launch") local

    frame ^[(move %0 arguments (izero %3 local) local) (move %1 arguments %limit local)]
    tailcall spawn_processes/2
.end

.function: main/1
    allocate_registers %3 local

    frame ^[(copy %0 arguments (integer %1 local 4000) local)]
    process %1 local process_spawner/1

    join void %1 local

    print (text %2 local "main/1 exited") local

    izero %0 local
    return
.end
