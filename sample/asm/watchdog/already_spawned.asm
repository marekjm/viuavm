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

.function: watchdog_process/1
    arg (.name: %iota death_message) %0

    .name: %iota exception
    remove %exception %1 (strstore %exception "exception")

    .name: %iota aborted_function
    remove %aborted_function %1 (strstore %aborted_function "function")

    echo (strstore (.name: %iota message) "process spawned with <")
    echo %aborted_function
    echo (strstore %message "> killed by >>>")
    echo %exception
    print (strstore %message "<<<")

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
    throw (istore %1 42)
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
