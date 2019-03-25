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

.function: watchdog_process/1
    allocate_registers %5 local

    move (.name: %iota death_message) local %0 parameters

    .name: %iota exception
    structremove %exception %1 (atom %exception 'exception')

    .name: %iota aborted_function
    structremove %aborted_function %1 (atom %aborted_function 'function')

    echo (string (.name: %iota message) local "process spawned with <") local
    echo %aborted_function local
    echo (string %message local "> killed by >>>") local
    echo %exception local
    print (string %message local "<<<") local

    return
.end

.function: broken_process/0
    allocate_registers %2 local

    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    throw (integer %1 local 42) local
    return
.end

.function: main/1
    allocate_registers %1 local

    frame %1
    watchdog watchdog_process/1

    frame %1
    watchdog watchdog_process/1

    frame %0
    process void broken_process/0

    izero %0 local
    return
.end
