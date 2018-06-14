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
    .name: %iota vec

    .name: %iota minus_one
    .name: %iota minus_two
    integer %minus_one local -1
    integer %minus_two local -2

    .name: %iota tmp
    if (not (isnull %tmp local %vec local) local) local logic
    vpush (vpush (vector %vec local) local (integer %tmp local 1) local) local (integer %tmp local 1) local

    .mark: logic

    .name: %iota number
    .name: %iota length
    arg %number local %0

    .mark: loop
    .name: %iota result
    if (not (lt %iota local (vlen %length local %vec local) local %number local) local) local finished
    add %result local *(vat %iota local %vec local %minus_one local) local *(vat %iota local %vec local %minus_two local) local
    vpush %vec local %result local
    jump loop

    .mark: finished
    copy %0 local *(vat %iota local %vec local %minus_one local) local
    return
.end

.function: main/1
    ; expected result is 1134903170

    .name: 2 result
    frame ^[(param %0 (integer %1 local 45) local)]
    print (call %result local iterfib/1) local

    izero %0 local
    return
.end
