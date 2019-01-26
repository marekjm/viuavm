;
;   Copyright (C) 2016, 2017, 2018 Marek Marecki
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
    allocate_registers %6 local

    frame ^[(move %0 arguments (integer %1 local 64) local)]
    call std::misc::cycle/1

    .name: %iota hello
    .name: %iota exclamation_mark
    text %hello local "Hello "
    text %exclamation_mark local "!"

    .name: %iota something
    move %something local %0 parameters

    .name: %iota textified
    text %textified local %something local

    .name: %iota to_print
    textconcat %to_print local %hello local %textified local
    textconcat %to_print local %to_print local %exclamation_mark local

    print %to_print local

    return
.end

.function: process_spawner/1
    allocate_registers %2 local

    frame ^[(move %0 arguments (move %1 local %0 parameters) local)]
    process void print_hello/1
    return
.end

.function: spawn_processes/1
    allocate_registers %2 local

    .name: 1 limit
    move %limit local %0 parameters

    ; run until "limit" hits zero
    if %limit local +1 spawn_processes/1__epilogue

    ; spawn a printer process with current limit value
    ; as its only parameter
    frame ^[(copy %0 arguments %limit local)]
    call process_spawner/1
    idec %limit local

    ; tail-recursive call to spawn more printer processes
    frame ^[(move %0 arguments %limit local)]
    tailcall spawn_processes/1

    .mark: spawn_processes/1__epilogue
    return
.end

.function: main/0
    allocate_registers %2 local

    ; spawn several processes, each printing a different "Hello {number}!"
    ; the hellos do not have to appear in the order their functions are
    ; called if there are multiple VP schedulers spawned
    ;
    ; this program is embarrassingly simple - it's just prints, but there is
    ; so many of them that the first VP scheduler starts to feel overwhelmed and
    ; calls for help by posting the processes to the CPU to let other schedulers
    ; adopt them
    ;
    ; so what this program does is saturate the schedulers

    import std::misc

    .name: 1 limit
    integer %limit local 64
    frame ^[(move %0 arguments %limit local)]
    call spawn_processes/1

    izero %0 local
    return
.end
