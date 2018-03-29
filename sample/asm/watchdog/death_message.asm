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

    arg %death_message local %0
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

.function: a_joined_concurrent_process/0
    frame ^[(pamv %0 (integer %1 128))]
    call std::misc::cycle/1

    print (string %1 "Hello World (from joined process)!")

    frame ^[(pamv %0 (string %1 "a_joined_concurrent_process"))]
    call log_exiting_joined/1

    throw (string %2 "OH NOES!")

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
.function: log_exiting_joined/1
    arg %1 %0
    echo (string %2 "process [ joined ]: '")
    echo %1
    print (string %2 "' exiting")
    return
.end

.signature: std::misc::cycle/1

.function: main/1
    import "std::misc"

    watchdog watchdog_process/1

    frame %0
    process void a_detached_concurrent_process/0

    frame %0
    process %2 a_joined_concurrent_process/0
    join void %2

    frame %0
    call log_exiting_main/0

    izero %0 local
    return
.end
