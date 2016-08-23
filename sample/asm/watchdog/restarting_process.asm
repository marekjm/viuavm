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

.function: watchdog_process/0
    .mark: __begin
    .name: 1 death_message
    receive death_message

    .name: 2 exception
    remove exception 1 (strstore exception "exception")

    .name: 3 function
    remove function 1 (strstore function "function")

    .name: 4 parameters
    remove parameters 1 (strstore parameters "parameters")

    echo (strstore 5 "[WARNING] process '")
    echo function
    echo parameters
    echo (strstore 5 "' killed by >>>")
    echo exception
    print (strstore 5 "<<<")

    ; handle only a_division_executing_process
    frame ^[(param 0 (vat 5 parameters 0)) (param 1 (iinc (vat 6 parameters 1)))]
    ;call 0 a_division_executing_process
    process 0 a_division_executing_process/2

    jump __begin

    return
.end

.function: a_detached_concurrent_process/0
    frame ^[(pamv 0 (istore 1 32))]
    call std::misc::cycle/1

    print (strstore 1 "Hello World (from detached process)!")

    frame ^[(pamv 0 (istore 1 512))]
    call std::misc::cycle/1

    print (strstore 1 "Hello World (from detached process) after a runaway exception!")

    frame ^[(pamv 0 (istore 1 512))]
    call std::misc::cycle/1

    frame ^[(pamv 0 (strstore 1 "a_detached_concurrent_process"))]
    call log_exiting_detached/1

    return
.end

.function: a_division_executing_process/2
    frame ^[(pamv 0 (istore 1 128))]
    call std::misc::cycle/1

    .name: 1 divide_what
    arg divide_what 0

    .name: 2 divide_by
    arg divide_by 1

    .name: 3 zero
    izero zero

    branch (ieq 4 divide_by zero) +1 __after_throw
    throw (strstore 4 "cannot divide by zero")
    .mark: __after_throw

    idiv 0 divide_what divide_by
    echo divide_what
    echo (strstore 4 ' / ')
    echo divide_by
    echo (strstore 4 ' = ')
    print 0

    return
.end

.function: log_exiting_main/0
    print (strstore 2 "process [  main  ]: 'main' exiting")
    return
.end
.function: log_exiting_detached/1
    arg 1 0
    echo (strstore 2 "process [detached]: '")
    echo 1
    print (strstore 2 "' exiting")
    return
.end
.function: log_exiting_joined/0
    arg 1 0
    echo (strstore 2 "process [ joined ]: '")
    echo 1
    print (strstore 2 "' exiting")
    return
.end

.signature: std::misc::cycle/1

.function: main/1
    link std::misc

    frame 0
    watchdog watchdog_process/0

    frame 0
    process 0 a_detached_concurrent_process/0

    frame ^[(param 0 (istore 3 42)) (param 1 (istore 4 0))]
    process 0 a_division_executing_process/2

    frame 0
    call log_exiting_main/0

    izero 0
    return
.end
