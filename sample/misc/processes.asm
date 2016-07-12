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

.function: run_in_a_process/1
    echo (strstore 1 "spawned process ")
    echo (arg 1 0)
    print (strstore 1 " exiting")
    return
.end

.function: spawn_processes/2
    .name: 1 counter
    arg counter 0

    .name: 2 limit
    arg limit 1

    ; if limit is N, processes IDs go from 0 to N-1
    ; this is why the counter is incremented before the
    ; branch instruction which checks for equality
    iinc counter

    ; if the counter is equal to limit return immediately
    branch (ieq 3 counter limit) +1 +2
    return

    ; spawn a new process
    frame ^[(param 0 counter)]
    process 5 run_in_a_process/1

    frame ^[(param 0 5)]
    msg 0 detach/1

    ; take advantage of tail recursion in Viua and
    ; elegantly follow with process spawner execution
    frame ^[(pamv 0 counter) (pamv 1 limit)]
    tailcall spawn_processes/2
.end

.function: process_spawner/1
    echo (strstore 2 "process_spawner/1: ")
    .name: 1 limit
    echo (arg 1 0)
    print (strstore 2 " processs to launch")

    frame ^[(pamv 0 (izero 3)) (pamv 1 limit)]
    tailcall spawn_processes/2
.end

.function: main/1
    frame ^[(param 0 (istore 1 100000))]
    process 1 process_spawner/1

    frame ^[(param 0 1) (param 1 (istore 2 512))]
    msg 0 setPriority/2

    join 0 1

    print (strstore 2 "main/1 exited")

    izero 0
    return
.end
