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

.function: process_to_suspend
    .name: 1 i
    istore 1 40
    .name: 2 m
    strstore 2 "iterations left (TTS): "
    .mark: __process_to_suspend_begin_while_0

    if 1 +1 __process_to_suspend_end_while_0

    echo 2
    nop
    nop
    idec i 
    nop
    print 1
    jump __process_to_suspend_begin_while_0

    .mark: __process_to_suspend_end_while_0

    print (strstore 1 "process_to_suspend/0 returned")
    return
.end

.function: process_to_do_the_suspending
    frame ^[(param 0 (arg 1 0)]
    msg void detach/1

    istore 4 10
    strstore 5 "iterations left (TDD): "
    .mark: loop_begin_0
    if 4 +1 loop_end_0
    echo 5
    nop
    nop
    nop
    nop
    idec 4
    print 4
    jump loop_begin_0
    .mark: loop_end_0

    frame ^[(param 0 1)]
    msg void suspend

    frame ^[(param 0 1]
    msg 2 suspended

    print 2
    istore 4 10
    strstore 5 "iterations left (TDD): "
    .mark: loop_begin
    if 4 +1 loop_end
    echo 5
    nop
    nop
    nop
    nop
    idec 4
    print 4
    jump loop_begin
    .mark: loop_end

    frame ^[(param 0 1)]
    msg void wakeup

    frame ^[(param 0 1]
    msg 2 suspended
    print 2

    return
.end

.function: main/1
    frame 0
    process 1 process_to_suspend
    
    frame ^[(param 0 (ptr 2 1))]
    process 3 process_to_do_the_suspending
    join 0 3

    print (strstore 5 "main/1 returned")
    izero 0
    return
.end
