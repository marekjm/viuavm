;
;   Copyright (C) 2016 Marek Marecki
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

.signature: printer::print/1
.signature: sleeper::lazy_print/0


.function: printer_wrapper/1
    -- just print the parameter received
    frame ^[(pamv 0 (arg 1 0))]
    call printer::print/1

    return
.end

.function: process_spawner/1
    -- call printer::print/1 in a new process to
    -- not block the execution
    frame ^[(pamv 0 (arg 1 0))]
    process 1 printer_wrapper/1

    -- detach the process since
    -- we do not care about its return value
    frame ^[(param 0 1)]
    msg 0 detach/1

    return
.end

.function: lazy_print_process/0
    frame 0
    call sleeper::lazy_print/0
    return
.end
.function: lazy_print_spawner/0
    frame 0
    process 1 lazy_print_process/0

    frame ^[(param 0 1)]
    msg 0 detach/1

    return
.end

.function: main/0
    -- link foreign printer module
    import "build/test/printer"
    import "build/test/sleeper"

    frame 0
    call lazy_print_spawner/0

    -- spawn several processes, each printing a different "Hello {who}!"
    -- the hellos are printed by foreign functions
    -- this test demonstrates that with more than one FFI scheduler several
    -- foreign function may execute in parallel
    frame ^[(pamv 0 (strstore 1 "Joe"))]
    call process_spawner/1

    frame ^[(pamv 0 (strstore 1 "Robert"))]
    call process_spawner/1

    frame ^[(pamv 0 (strstore 1 "Mike"))]
    call process_spawner/1

    frame ^[(pamv 0 (strstore 1 "Bjarne"))]
    call process_spawner/1

    frame ^[(pamv 0 (strstore 1 "Guido"))]
    call process_spawner/1

    frame ^[(pamv 0 (strstore 1 "Dennis"))]
    call process_spawner/1

    frame ^[(pamv 0 (strstore 1 "Bram"))]
    call process_spawner/1

    frame ^[(pamv 0 (strstore 1 "Herb"))]
    call process_spawner/1

    frame ^[(pamv 0 (strstore 1 "Anthony"))]
    call process_spawner/1

    frame ^[(pamv 0 (strstore 1 "Alan"))]
    call process_spawner/1

    frame ^[(pamv 0 (strstore 1 "Ada"))]
    call process_spawner/1

    frame ^[(pamv 0 (strstore 1 "Charles"))]
    call process_spawner/1

    izero 0
    return
.end
