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
    integer %0 local 99

    string %1 local " bottles"
    string %2 local " of beer"
    string %3 local " on the wall"
    string %4 local "Take one down, pass it around"
    string %5 local ""
    string %6 local "No more"

    integer %7 local 1
    jump again

.mark: one_beer
    string %1 local " bottle"

.mark: again

    echo %0 local
    echo %1 local
    echo %2 local
    print %3 local

    echo %0 local
    echo %1 local
    print %2 local

    print %4 local

    idec %0 local

    if %0 local more_beer
    echo %6 local
    jump rest

.mark: more_beer
    echo %0 local
    jump rest

.mark: rest
    echo %1 local
    echo %2 local
    print %3 local

    print %5 local

    eq %8 local %7 local %0 local
    if %8 local one_beer
    if %0 local again

    izero %0 local
    return
.end
