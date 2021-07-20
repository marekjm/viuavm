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

.function: watchdog_process/2
    allocate_registers %8 local

    .name: %iota death_message
    .name: %iota ex
    .name: %iota aborted_function
    .name: %iota params

    move %death_message local %0 parameters
    structremove %ex local %death_message local (atom %ex local 'exception') local
    structremove %aborted_function local %death_message local (atom %aborted_function local 'function') local
    structremove %params local %death_message local (atom %params local 'parameters') local

    .name: %iota message
    echo (string %message local "[WARNING] process '") local
    echo %aborted_function local
    echo %params local
    echo (string %message local "' killed by >>>") local
    echo %ex local
    print (string %message local "<<<") local

    .name: iota i
    move %i local %1 parameters

    frame %2
    move %0 arguments %i local
    move %1 arguments (integer %iota local 1) local
    process void a_division_executing_process/2

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

.function: formatting/2
    allocate_registers %5 local

    move (.name: %iota divide_what) local %0 parameters
    move (.name: %iota divide_by) local %1 parameters

    text %divide_what local %divide_what local
    text %divide_by local %divide_by local
    .name: %iota divide_op
    text %divide_op local " / "

    .name: %iota result
    textconcat %result local %divide_what local %divide_op local
    textconcat %result local %result local %divide_by local

    return
.end
.function: a_division_executing_process/2
    allocate_registers %5 local

    .name: iota divide_what
    move %divide_what local %0 parameters

    .name: iota divide_by
    move %divide_by local %1 parameters

    .name: iota zero
    izero %zero local

    frame %2
    copy %1 arguments %divide_what local
    watchdog watchdog_process/2

    frame ^[(move %0 arguments (integer %4 local 128) local)]
    call std::misc::cycle/1

    if (eq %4 local %divide_by local %zero local) local +1 __after_throw
    throw (string %4 local "cannot divide by zero") local
    .mark: __after_throw

    div %0 local %divide_what local %divide_by local
    echo %divide_what local
    echo (string %4 local ' / ') local
    echo %divide_by local
    echo (string %4 local ' = ') local
    print %0 local

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

.import: [[dynamic]] std::misc

.function: main/1
    allocate_registers %3 local

    frame %0
    process void a_detached_concurrent_process/0

    frame ^[(copy %0 arguments (integer %1 local 42) local) (copy %1 arguments (integer %2 local 0) local)]
    process void a_division_executing_process/2

    frame %0
    call log_exiting_main/0

    izero %0 local
    return
.end
