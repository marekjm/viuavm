;
;   Copyright (C) 2015, 2016, 2017 Marek Marecki
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

    integer %number local 166737
    integer %modulo local 176

    copy %result local %number local

    ; if number is less than modulo, jump straight to result printing
    lt %4 local %number local %modulo local
    if %4 local final

    ; otherwise we must perform some calculations
    div %number local %number local %modulo local
    mul %number local %number local %modulo local
    sub %result local %result local %number local


    .mark: final
    print %result local
    izero %0 local
    return
.end
