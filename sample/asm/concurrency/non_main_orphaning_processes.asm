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

.function: print_lazy/1
    ; many nops to make the process run for a long time long
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    print (arg 1 0)
    return
.end

.function: print_eager/1
    print (arg 1 0)
    return
.end

.function: process_spawner/0
    frame ^[(param 0 (strstore 1 "Hello concurrent World! (1)"))]
    process 3 print_lazy/1

    frame ^[(param 0 (strstore 2 "Hello concurrent World! (2)"))]
    process 4 print_eager/1

    join 0 4
    ; do not join the process to test main/1 node orphaning detection
    ;join 0 3

    return
.end

.function: main/1
    frame 0
    call process_spawner/0

    izero 0
    return
.end
