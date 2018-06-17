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
    integer %1 local 80

    string %3 local "iterations left: "

    .mark: __will_be_killed_by_a_runaway_exception_begin_while_0
    if %1 local +1 __will_be_killed_by_a_runaway_exception_end_while_1
    echo %3 local
    print (idec %1 local) local
    jump __will_be_killed_by_a_runaway_exception_begin_while_0
    .mark: __will_be_killed_by_a_runaway_exception_end_while_1

    throw (string %2 local "Hello runaway World!") local

    return
.end

.function: std::util::cpu::cycle/1
    ; executes at least N cycles
    ;
    .name: 1 counter
    arg %counter local %0

    .name: 4 i
    sub %counter local %counter local (integer %i local 9) local
    div %counter local %counter local (integer %i local 2) local

    .name: 2 zero
    izero %zero local

    .mark: __loop_begin
    if (lte %3 local %counter local %zero local) local __loop_end +1
    idec %counter local
    jump __loop_begin
    .mark: __loop_end

    return
.end

.function: main/1
    watchdog supervisor_function/0

    print (string %3 local "main/1 exiting") local
    izero %0 local
    return
.end
