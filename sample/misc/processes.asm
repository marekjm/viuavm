;
;   Copyright (C) 2015, 2016, 2017 Marek Marecki
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
    arg %1 local %0

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
    .name: 1 counter
    arg %counter %0

    .name: 2 limit
    arg %limit %1

    ; if limit is N, processes IDs go from 0 to N-1
    ; this is why the counter is incremented before the
    ; branch instruction which checks for equality
    iinc %counter

    ; if the counter is equal to limit return immediately
    if (eq %3 %counter %limit) +1 +2
    return

    ; spawn a new process
    frame ^[(param %0 %counter)]
    process void run_in_a_process/1

    ; take advantage of tail recursion in Viua and
    ; elegantly follow with process spawner execution
    frame ^[(pamv %0 %counter) (pamv %1 %limit)]
    tailcall spawn_processes/2
.end

.function: process_spawner/1
    echo (string %2 "process_spawner/1: ")
    .name: 1 limit
    echo (arg %1 %0)
    print (string %2 " processs to launch")

    frame ^[(pamv %0 (izero %3)) (pamv %1 %limit)]
    tailcall spawn_processes/2
.end

.function: main/1
    frame ^[(param %0 (integer %1 4000))]
    process %1 process_spawner/1

    join %0 %1

    print (string %2 "main/1 exited")

    izero %0 local
    return
.end
