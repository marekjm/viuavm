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

.function: iterfib/1
    .name: %iota vector

    ress static

    .name: %iota minus_one
    .name: %iota minus_two
    istore %minus_one -1
    istore %minus_two -2

    .name: %iota tmp
    if (not (isnull %tmp %vector)) logic
    vpush (vpush (vec %vector) (istore %tmp 1)) (istore %tmp 1)

    .mark: logic

    .name: %iota number
    .name: %iota length
    arg %number %0

    .mark: loop
    .name: %iota result
    if (not (lt int64 %iota (vlen %length %vector) %number)) finished
    add int64 %result *(vat %iota %vector %minus_one) *(vat %iota %vector %minus_two)
    vpush %vector %result
    jump loop

    .mark: finished
    copy %0 *(vat %iota %vector %minus_one)
    return
.end

.function: main/1
    ; expected result is 1134903170

    .name: 2 result
    frame ^[(param %0 (istore %1 45))]
    print (call %result iterfib/1)

    izero %0 local
    return
.end
