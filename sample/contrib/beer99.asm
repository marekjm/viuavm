; 99 bottles of beer program for Viua VM Assembler (modified)
; Copyright (C) 2015  Marek Marecki
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.

.function: bottles_on_the_wall
    strstore 1 " bottles"
    strstore 2 " of beer"
    strstore 3 " on the wall"

    arg 0 0

    istore 4 1
    ieq 4 4 0

    if 4 +1 +2
    strstore 1 " bottle"

    izero 4
    ieq 4 4 0
    if 4 zero_bottles

    echo 0
    echo 1
    echo 2
    print 3
    jump quit

    .mark: zero_bottles
    strstore 1 "No more bottles of beer on the wall"
    print 1

    .mark: quit
    return
.end

.function: bottles
    strstore 1 " bottles"
    strstore 2 " of beer"

    arg 0 0

    istore 4 1
    ieq 4 4 0

    if 4 +1 +2
    strstore 1 " bottle"

    echo 0
    echo 1
    print 2

    idec 0
    return
.end

.function: report_state_of_the_wall
    arg 0 0
    strstore 4 "Take one down, pass it around"
    strstore 5 ""

    frame 1
    param 0 0
    call bottles_on_the_wall

    frame 1
    ; co... co... co... co... COMBOBREAKER!!!1!1!
    ; DOUBLE pass by reference - "bottles" function will *really* decrement the counter!
    ; we're showing off, so... why not?
    paref 0 0
    call bottles

    print 4

    frame 1
    param 0 0
    call bottles_on_the_wall

    print 5

    return
.end

.function: main/1
    istore 0 99

    istore 7 1
    jump again

    .mark: again
    frame 1
    ; pass by reference - "report_state_of_the_wall" function decrements the counter!
    paref 0 0
    call report_state_of_the_wall

    if 0 again

    izero 0
    return
.end
