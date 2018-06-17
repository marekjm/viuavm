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

.function: fibonacci/2
    .name: 1 current_value
    .name: 2 accumulator
    arg %current_value local %0
    arg %accumulator local %1

    if %current_value local +1 fibonacci/2__finished

    add %accumulator local %current_value local

    frame ^[(pamv %0 (idec %current_value local) local) (pamv %1 %accumulator local)]
    tailcall fibonacci/2

    .mark: fibonacci/2__finished
    move %0 local %accumulator local
    return
.end

.function: fibonacci/1
    .name: 2 accumulator

    frame ^[(pamv %0 (arg %1 local %0) local) (pamv %1 (izero %accumulator local) local)]
    call %accumulator local fibonacci/2

    move %0 local %accumulator local
    return
.end

.function: main/0
    .name: 1 result

    integer %result local 5

    frame ^[(pamv %0 %result local)]
    call %result local fibonacci/1

    print %result local

    izero %0 local
    return
.end
