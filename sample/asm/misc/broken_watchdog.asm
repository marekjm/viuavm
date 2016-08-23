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

.function: supervisor_function/0
    ; uncomment the `receive 1` and
    ; the program is no longer broken
    ;receive 1
    -- this program tests if the machine can quit if
    -- watchdog is pathologically broken
    --
    -- this test will fail if this watchdog function gets
    -- at least one message
    return
.end

.function: will_be_killed_by_a_runaway_exception/0
    istore 1 80

    strstore 3 "iterations left: "

    .mark: __will_be_killed_by_a_runaway_exception_begin_while_0
    branch 1 +1 __will_be_killed_by_a_runaway_exception_end_while_1
    echo 3
    print (idec 1)
    jump __will_be_killed_by_a_runaway_exception_begin_while_0
    .mark: __will_be_killed_by_a_runaway_exception_end_while_1

    throw (strstore 2 "Hello runaway World!")

    return
.end

.function: std::util::cpu::cycle/1
    ; executes at least N cycles
    ;
    .name: 1 counter
    arg counter 0

    istore 2 9
    isub counter counter 2
    istore 2 2
    idiv counter counter 2

    .name: 2 zero
    izero zero

    .mark: __loop_begin
    branch (ilte 3 counter zero) __loop_end +1
    idec counter
    jump __loop_begin
    .mark: __loop_end

    return
.end

.function: main/1
    frame 0
    watchdog supervisor_function/0

    ;frame 0
    ;process 1 will_be_killed_by_a_runaway_exception
    ;ptr 2 1
    ;frame ^[(param 0 2)]
    ;msg 0 detach/1

    ;frame ^[(pamv 0 (istore 1 1024))]
    ;call std::util::cpu::cycle/1

    print (strstore 3 "main/1 exiting")
    izero 0
    return
.end
