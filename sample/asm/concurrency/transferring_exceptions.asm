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

.function: run_in_a_process/0
    allocate_registers %2 local

    ; FIXME will cause a memory leak on detached processes
    throw (receive %1 local) local
    return
.end

.block: try_process_exception
    join void %1 local
    leave
.end

.block: handle_process_exception
    echo (string %3 local "exception transferred from process ") local
    echo %1 local
    echo (string %3 local ": ") local
    print (draw %3 local) local
    leave
.end

.function: main/1
    allocate_registers %4 local

    frame %0
    process %1 local run_in_a_process/0

    send %1 local (string %2 local "Hello exception transferring World!") local

    try
    catch "String" handle_process_exception
    enter try_process_exception

    izero %0 local
    return
.end
