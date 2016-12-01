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

.function: run_in_a_process/0
    ; will cause a memory leak on detached processes
    throw (receive 1)
    return
.end

.block: try_process_exception
    join 0 1
    leave
.end

.block: handle_process_exception
    echo (strstore 3 "exception transferred from process ")
    echo 1
    echo (strstore 3 ": ")
    print (draw 3)
    leave
.end

.function: main/1
    frame 0
    process 1 run_in_a_process/0

    ;frame ^[(param 0 1)]
    ;msg 0 detach/1

    send 1 (strstore 2 "Hello exception transferring World!")

    try
    catch "String" handle_process_exception
    enter try_process_exception

    izero 0
    return
.end
