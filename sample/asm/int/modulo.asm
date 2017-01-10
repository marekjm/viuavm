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
    .name: 1 number
    .name: 2 modulo
    .name: 3 result

    istore %number 166737
    istore %modulo 176

    copy %result %number

    ; if number is less than modulo, jump straight to result printing
    lt int64 %4 %number %modulo
    if %4 final

    ; otherwise we must perform some calculations
    div int64 %number %number %modulo
    mul int64 %number %number %modulo
    sub int64 %result %result %number


    .mark: final
    print %result
    izero %0
    return
.end
