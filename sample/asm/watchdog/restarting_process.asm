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
    .name: %iota death_message
    .name: %iota exception
    .name: %iota aborted_function
    .name: %iota parameters

    arg %death_message local %0
    structremove %exception local %death_message local (atom %exception local 'exception') local
    structremove %aborted_function local %death_message local (atom %aborted_function local 'function') local
    structremove %parameters local %death_message local (atom %parameters local 'parameters') local

    .name: %iota message
    echo (string %message local "[WARNING] process '") local
    echo %aborted_function local
    echo %parameters local
    echo (string %message local "' killed by >>>") local
    echo %exception local
    print (string %message local "<<<") local

    .name: %iota i
    vat %i local %parameters local (integer %iota local 1) local
    copy %i local *i local
    vat %message local %parameters local (integer %iota local 0) local
    frame ^[(param %0 *message local) (param %1 (iinc %i local) local)]
    process void a_division_executing_process/2

    return
.end

.function: a_detached_concurrent_process/0
    watchdog watchdog_process/1

    frame ^[(pamv %0 (integer %1 32))]
    call std::misc::cycle/1

    print (string %1 "Hello World (from detached process)!")

    frame ^[(pamv %0 (integer %1 512))]
    call std::misc::cycle/1

    print (string %1 "Hello World (from detached process) after a runaway exception!")

    frame ^[(pamv %0 (integer %1 512))]
    call std::misc::cycle/1

    frame ^[(pamv %0 (string %1 "a_detached_concurrent_process"))]
    call log_exiting_detached/1

    return
.end

.function: formatting/2
    arg (.name: %iota divide_what) %0
    arg (.name: %iota divide_by) %1

    string (.name: %iota format_string) "#{0} / #{1}"
    frame ^[(param %0 %format_string) (param %1 %divide_what) (param %2 %divide_by)]
    move %0 (msg %iota format/)

    return
.end
.function: a_division_executing_process/2
    watchdog watchdog_process/1

    frame ^[(pamv %0 (integer %1 128))]
    call std::misc::cycle/1

    .name: 1 divide_what
    arg %divide_what %0

    .name: 2 divide_by
    arg %divide_by %1

    .name: 3 zero
    izero %zero

    if (eq %4 %divide_by %zero) +1 __after_throw
    throw (string %4 "cannot divide by zero")
    .mark: __after_throw

    div %0 %divide_what %divide_by
    echo %divide_what
    echo (string %4 ' / ')
    echo %divide_by
    echo (string %4 ' = ')
    print %0

    return
.end

.function: log_exiting_main/0
    print (string %2 "process [  main  ]: 'main' exiting")
    return
.end
.function: log_exiting_detached/1
    arg %1 %0
    echo (string %2 "process [detached]: '")
    echo %1
    print (string %2 "' exiting")
    return
.end
.function: log_exiting_joined/0
    arg %1 %0
    echo (string %2 "process [ joined ]: '")
    echo %1
    print (string %2 "' exiting")
    return
.end

.signature: std::misc::cycle/1

.function: main/1
    import "std::misc"

    frame %0
    process void a_detached_concurrent_process/0

    frame ^[(param %0 (integer %3 42)) (param %1 (integer %4 0))]
    process void a_division_executing_process/2

    frame %0
    call log_exiting_main/0

    izero %0 local
    return
.end
