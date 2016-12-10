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

.function: main/1
    vinsert (vinsert (vinsert (vinsert (vec 1) (strstore 5 "sheep!")) (strstore 2 "Hurr")) (strstore 3 "durr") 1) (strstore 4 "Im'a") 2

    ; this works OK
    ; this instruction is here for debugging - uncomment it to see the vector
    ;print 1

    .name: 6 len
    .name: 7 counter

    istore counter 0
    vlen len 1

    .mark: loop
    if (lt int64 8 counter len) +1 break
    print *(vat 9 1 @counter)
    iinc counter
    jump loop

    .mark: break

    ; see comment for first vector-print
    ; it is for debugging - this should work without segfault
    ;print 1

    izero 0
    return
.end
