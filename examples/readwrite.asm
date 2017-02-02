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

; Example file showing more involved interfacing with C++
;
; Compilation:
;
;   g++ -std=c++11 -Wall -pedantic -fPIC -c -I../include -o registerset.o ../src/cpu/registerset.cpp
;   g++ -std=c++11 -Wall -pedantic -fPIC -shared -I../include -o io.so io.cpp registerset.o
;   ../build/bin/vm/asm readwrite.asm
;
; After libraries and the program are compiled result can be tested with following command
; which will ask for a path to read, read it, ask for a path write and write the string to it.
;
;   ../build/bin/vm/cpu a.out
;

.signature: io::getline/0
.signature: io::read/1
.signature: io::write/2

.function: main/0
    import "./io"

    echo (strstore %1 "Path to read: ")
    frame %0
    print (call %1 io::getline/0)

    frame ^[(param %0 %1)]
    echo (call %2 io::read/1)

    echo (strstore %3 "Path to write: ")
    frame %0
    call %3 io::getline/0

    frame ^[(param %0 %3) (param %1 %2)]
    call void io::write/2

    izero %0
    return
.end
