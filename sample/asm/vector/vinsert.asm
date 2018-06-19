;
;   Copyright (C) 2015, 2016, 2017, 2018 Marek Marecki
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
    allocate_registers %10 local

    vector %1 local

    integer %0 local 0
    vinsert %1 local (string %5 local "sheep!") local %0 local
    vinsert %1 local (string %5 local "Hurr") local %0 local

    integer %0 local 1
    vinsert %1 local (string %5 local "durr") local %0 local

    integer %0 local 2
    vinsert %1 local (string %5 local "Im'a") local %0 local

    .name: 6 len
    .name: 7 counter

    integer %counter local 0
    vlen %len local %1 local

    .mark: loop
    if (lt %8 local %counter local %len local) local +1 break
    print *(vat %9 local %1 local %counter local) local
    iinc %counter local
    jump loop

    .mark: break

    izero %0 local
    return
.end
