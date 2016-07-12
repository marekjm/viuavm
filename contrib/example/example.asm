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


; signatures from foreign library
.signature: example::hello_world/0
.signature: example::hello/1
.signature: example::what_time_is_it/0

.block: __catch_0_main_Exception
    echo (strstore 1 "fail: ")
    print (pull 1)
    leave
.end
.block: __try_0_main
    frame 0
    call example::hello_world
    leave
.end

.function: main/0
    ; the foreign library must be imported
    import "example"


    ; exceptions thrown by foreign libraries can be caught as usual
    try
    catch "Exception" __catch_0_main_Exception
    enter __try_0_main


    ; call with a parameter
    frame ^[(param 0 (strstore 1 "Joe"))]
    call example::hello/1


    ; call with a return value
    frame 0
    call 2 example::what_time_is_it/0
    echo (strstore 1 "The time (in seconds since epoch) is: ")
    print 2

    izero 0
    return
.end
