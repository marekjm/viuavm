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
.signature: printer::format/1
.signature: sleeper::lazy_print/0


.function: printer/0
    allocate_registers %4 local

    receive %1 local infinity
    atom %2 local 'has_value'
    structat %3 local %1 local %2 local

    if *3 local +1 the_end
    atom %2 local 'value'
    structat %3 local %1 local %2 local
    frame %1
    copy %0 arguments *3 local
    call void printer::print/1

    frame %0
    tailcall printer/0

    .mark: the_end
    return
.end

.function: printer_wrapper/2
    allocate_registers %6 local

    .name: iota name
    .name: iota pid_of_printer

    move %name local %0 parameters
    move %pid_of_printer local %1 parameters

    struct %3 local

    atom %4 local 'has_value'
    izero %5 local
    not %5 local
    structinsert %3 local %4 local %5 local

    atom %4 local 'value'
    structinsert %3 local %4 local %name local

    send %pid_of_printer local %3 local

    return
.end

.function: process_spawner/2
    allocate_registers %3 local

    .name: iota name
    .name: iota pid_of_printer

    move %name local %0 parameters
    move %pid_of_printer local %1 parameters

    frame %2
    move %0 arguments %name local
    move %1 arguments %pid_of_printer local
    process void printer_wrapper/2

    return
.end

.function: lazy_print_process/0
    allocate_registers %1 local

    frame %0
    call void sleeper::lazy_print/0
    return
.end
.function: lazy_print_spawner/0
    allocate_registers %1 local

    frame %0
    process void lazy_print_process/0
    return
.end

.function: main/0
    allocate_registers %6 local

    ; import "foreign printer module"
    import printer
    import sleeper

    frame %0
    call void lazy_print_spawner/0

    frame %0
    process %2 local printer/0

    ; spawn several processes, each printing a different "Hello {who}!"
    ; the hellos are printed by foreign functions
    ; this test demonstrates that with more than one FFI scheduler several
    ; foreign function may execute in parallel
    frame ^[(move %0 arguments (text %1 local "Joe") local) (copy %1 arguments %2 local)]
    call void process_spawner/2

    frame ^[(move %0 arguments (text %1 local "Robert") local) (copy %1 arguments %2 local)]
    call void process_spawner/2

    frame ^[(move %0 arguments (text %1 local "Mike") local) (copy %1 arguments %2 local)]
    call void process_spawner/2

    frame ^[(move %0 arguments (text %1 local "Bjarne") local) (copy %1 arguments %2 local)]
    call void process_spawner/2

    frame ^[(move %0 arguments (text %1 local "Guido") local) (copy %1 arguments %2 local)]
    call void process_spawner/2

    frame ^[(move %0 arguments (text %1 local "Dennis") local) (copy %1 arguments %2 local)]
    call void process_spawner/2

    frame ^[(move %0 arguments (text %1 local "Bram") local) (copy %1 arguments %2 local)]
    call void process_spawner/2

    frame ^[(move %0 arguments (text %1 local "Anthony") local) (copy %1 arguments %2 local)]
    call void process_spawner/2

    frame ^[(move %0 arguments (text %1 local "Alan") local) (copy %1 arguments %2 local)]
    call void process_spawner/2

    frame ^[(move %0 arguments (text %1 local "Ada") local) (copy %1 arguments %2 local)]
    call void process_spawner/2

    frame ^[(move %0 arguments (text %1 local "Charles") local) (copy %1 arguments %2 local)]
    call void process_spawner/2

    integer %3 local 0
    integer %5 local 42
    io_read %4 local %3 local %5 local
    try
    catch "Exception" .block: catch_0
        draw %5 local
        delete %5 local
        leave
    .end
    enter .block: enter_0
        io_wait void %4 local 10ms
        leave
    .end
    io_cancel %4 local
    delete %4 local
    delete %3 local

    struct %3 local
    atom %4 local 'has_value'
    izero %5 local
    not %5 local
    not %5 local
    structinsert %3 local %4 local %5 local
    send %2 local %3 local

    join void %2 local infinity

    izero %0 local
    return
.end
