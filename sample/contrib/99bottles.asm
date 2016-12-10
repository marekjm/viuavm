; 99 bottles of beer program for Viua VM Assembler
; Copyright (C) 2015  Harald Eilertsen
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

.function: main/1
    istore 0 99

    strstore 1 " bottles"
    strstore 2 " of beer"
    strstore 3 " on the wall"
    strstore 4 "Take one down, pass it around"
    strstore 5 ""
    strstore 6 "No more"

    istore 7 1
    jump again

.mark: one_beer
    strstore 1 " bottle"

.mark: again

    echo 0
    echo 1
    echo 2
    print 3

    echo 0
    echo 1
    print 2

    print 4

    idec 0

    if 0 more_beer
    echo 6
    jump rest

.mark: more_beer
    echo 0
    jump rest

.mark: rest
    echo 1
    echo 2
    print 3

    print 5

    eq int64 8 7 0
    if 8 one_beer
    if 0 again

    izero 0
    return
.end
