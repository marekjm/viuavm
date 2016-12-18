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

.signature: std::io::getline/0
.signature: std::misc::cycle/1

.function: run_in_a_process/0
    print (strstore 1 "worker process: starting...")

    frame ^[(pamv 0 (istore 1 524288))]
    call std::misc::cycle/1

    print (strstore 1 "worker process: started")
    print (receive 1)

    frame ^[(pamv 0 (istore 1 524288))]
    call std::misc::cycle/1

    print (strstore 1 "worker process: stopped")
    return
.end

.function: main/1
    import "io"
    link std::misc

    frame 0
    process 1 run_in_a_process/0

    frame ^[(param 0 (ptr 2 1))]
    msg void detach/1
    print (strstore 2 "main/1 detached worker process")

    echo (strstore 2 "message to pass: ")
    frame 0
    call 2 std::io::getline/0

    frame ^[(param 0 1) (param 1 2)]
    msg void pass/2

    izero 0
    return
.end
