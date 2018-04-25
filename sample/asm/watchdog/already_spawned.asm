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

.function: watchdog_process/1
    arg (.name: %iota death_message) %0

    .name: %iota exception
    structremove %exception %1 (atom %exception 'exception')

    .name: %iota aborted_function
    structremove %aborted_function %1 (atom %aborted_function 'function')

    echo (string (.name: %iota message) "process spawned with <")
    echo %aborted_function
    echo (string %message "> killed by >>>")
    echo %exception
    print (string %message "<<<")

    return
.end

.function: broken_process/0
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
    throw (integer %1 42)
    return
.end

.function: main/1
    watchdog watchdog_process/1

    watchdog watchdog_process/1

    frame %0
    process void broken_process/0

    izero %0 local
    return
.end
