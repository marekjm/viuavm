;
;   Copyright (C) 2016 Marek Marecki
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

.signature: std::misc::cycle/1

.function: print_hello/1
    frame ^[(pamv 0 (istore 1 64))]
    call std::misc::cycle/1

    .name: 2 format_args
    vec format_args (arg 1 0) 1

    .name: 1 format_string
    strstore format_string "Hello #{0}!\n"

    frame ^[(param 0 format_string) (param 1 format_args)]
    msg 3 format/

    echo 3

    return
.end

.function: process_spawner/1
    frame ^[(pamv 0 (arg 1 0))]
    process void print_hello/1
    return
.end

.function: spawn_processes/1
    .name: 1 limit
    arg limit 0

    -- run until "limit" hits zero
    if limit +1 spawn_processes/1__epilogue

    -- spawn a printer process with current limit value
    -- as its only parameter
    frame ^[(param 0 limit)]
    call process_spawner/1
    idec limit

    -- tail-recursive call to spawn more printer processes
    frame ^[(pamv 0 limit)]
    tailcall spawn_processes/1

    .mark: spawn_processes/1__epilogue
    return
.end

.function: main/0
    -- spawn several processes, each printing a different "Hello {number}!"
    -- the hellos do not have to appear in the order their functions are
    -- called if there are multiple VP schedulers spawned
    --
    -- this program is embarrassingly simple - it's just prints, but there is
    -- so many of them that the first VP scheduler starts to feel overwhelmed and
    -- calls for help by posting the processes to the CPU to let other schedulers
    -- adopt them
    --
    -- so what this program does is saturate the schedulers

    link std::misc

    .name: 1 limit
    istore limit 64
    frame ^[(pamv 0 limit)]
    call spawn_processes/1

    izero 0
    return
.end
