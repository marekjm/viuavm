;
;   Copyright (C) 2016, 2017, 2018 Marek Marecki
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
    allocate_registers %2 local

    ; just print %the parameter received
    frame ^[(pamv %0 (arg %1 local %0) local)]
    call void printer::print/1

    return
.end

.function: process_spawner/1
    allocate_registers %2 local

    ; call printer::print/1 in a new process to
    ; not block the execution, and
    ; detach the process as we do not care about its return value
    frame ^[(pamv %0 (arg %1 local %0) local)]
    process void printer_wrapper/1
    return
.end

.function: lazy_print_process/0
    allocate_registers %0 local

    frame %0
    call void sleeper::lazy_print/0
    return
.end
.function: lazy_print_spawner/0
    allocate_registers %0 local

    frame %0
    process void lazy_print_process/0
    return
.end

.function: main/0
    allocate_registers %2 local

    ; import "foreign printer module"
    import "build/test/printer"
    import "build/test/sleeper"

    frame %0
    call void lazy_print_spawner/0

    ; spawn several processes, each printing a different "Hello {who}!"
    ; the hellos are printed by foreign functions
    ; this test demonstrates that with more than one FFI scheduler several
    ; foreign function may execute in parallel
    frame ^[(pamv %0 (text %1 local "Joe") local)]
    call void process_spawner/1

    frame ^[(pamv %0 (text %1 local "Robert") local)]
    call void process_spawner/1

    frame ^[(pamv %0 (text %1 local "Mike") local)]
    call void process_spawner/1

    frame ^[(pamv %0 (text %1 local "Bjarne") local)]
    call void process_spawner/1

    frame ^[(pamv %0 (text %1 local "Guido") local)]
    call void process_spawner/1

    frame ^[(pamv %0 (text %1 local "Dennis") local)]
    call void process_spawner/1

    frame ^[(pamv %0 (text %1 local "Bram") local)]
    call void process_spawner/1

    frame ^[(pamv %0 (text %1 local "Herb") local)]
    call void process_spawner/1

    frame ^[(pamv %0 (text %1 local "Anthony") local)]
    call void process_spawner/1

    frame ^[(pamv %0 (text %1 local "Alan") local)]
    call void process_spawner/1

    frame ^[(pamv %0 (text %1 local "Ada") local)]
    call void process_spawner/1

    frame ^[(pamv %0 (text %1 local "Charles") local)]
    call void process_spawner/1

    izero %0 local
    return
.end
