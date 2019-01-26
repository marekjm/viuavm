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

    .name: %iota death_message
    .name: %iota exception
    .name: %iota aborted_function

    move %death_message local %0 parameters
    structremove %exception local %death_message local (atom %exception local 'exception') local
    structremove %aborted_function local %death_message local (atom %aborted_function local 'function') local

    .name: %iota message
    echo (string %message local "[WARNING] process '") local
    echo %aborted_function local
    echo (string %message local "' killed by >>>") local
    echo %exception local
    print (string %message local "<<<") local

    return
.end

.function: a_detached_concurrent_process/0
    allocate_registers %2 local

    frame ^[(move %0 arguments (integer %1 local 32) local)]
    call std::misc::cycle/1

    print (string %1 local "Hello World (from detached process)!") local

    frame ^[(move %0 arguments (integer %1 local 512) local)]
    call std::misc::cycle/1

    print (string %1 local "Hello World (from detached process) after a runaway exception!") local

    frame ^[(move %0 arguments (integer %1 local 512) local)]
    call std::misc::cycle/1

    frame ^[(move %0 arguments (string %1 local "a_detached_concurrent_process") local)]
    call log_exiting_detached/1

    return
.end

.function: a_joined_concurrent_process/0
    allocate_registers %3 local

    frame ^[(move %0 arguments (integer %1 local 128) local)]
    call void std::misc::cycle/1

    print (string %1 local "Hello World (from joined process)!") local

    frame ^[(move %0 arguments (string %1 local "a_joined_concurrent_process") local)]
    call void log_exiting_joined/1

    throw (string %2 local "OH NOES!") local

    return
.end

.function: log_exiting_main/0
    allocate_registers %2 local

    print (string %1 local "process [  main  ]: 'main' exiting") local
    return
.end
.function: log_exiting_detached/1
    allocate_registers %3 local

    move %1 local %0 parameters
    echo (string %2 local "process [detached]: '") local
    echo %1 local
    print (string %2 local "' exiting") local
    return
.end
.function: log_exiting_joined/1
    allocate_registers %3 local

    move %1 local %0 parameters
    echo (string %2 local "process [ joined ]: '") local
    echo %1 local
    print (string %2 local "' exiting") local
    return
.end

.signature: std::misc::cycle/1

.function: main/1
    allocate_registers %2 local

    import std::misc

    watchdog watchdog_process/1

    frame %0
    process void a_detached_concurrent_process/0

    frame %0
    process %1 local a_joined_concurrent_process/0
    join void %1 local

    frame %0
    call log_exiting_main/0

    izero %0 local
    return
.end
