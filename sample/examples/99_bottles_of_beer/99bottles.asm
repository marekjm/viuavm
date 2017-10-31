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

;
;   EDIT #1, 2017, by Marek Marecki
;
;   Compile this with --no-sa option.
;   It is not a bug in Harald's code, but rather a limitation of
;   Viua's static analyser (it can't go "back" so it sees some registers as
;   unused while they would be in fact used at a later moment after
;   taking a jump).
;
;   EDIT #2, 2017, by Marek Marecki
;
;   Added 'local' after 'izero' at the end of main function to conform
;   to Viua spec.
;

.function: main/1
    istore %0 99

    string %1 " bottles"
    string %2 " of beer"
    string %3 " on the wall"
    string %4 "Take one down, pass it around"
    string %5 ""
    string %6 "No more"

    istore %7 1
    jump again

.mark: one_beer
    string %1 " bottle"

.mark: again

    echo %0
    echo %1
    echo %2
    print %3

    echo %0
    echo %1
    print %2

    print %4

    idec %0

    if %0 more_beer
    echo %6
    jump rest

.mark: more_beer
    echo %0
    jump rest

.mark: rest
    echo %1
    echo %2
    print %3

    print %5

    eq %8 %7 %0
    if %8 one_beer
    if %0 again

    izero %0 local
    return
.end
