;
;   Copyright (C) 2017, 2018 Marek Marecki <marekjm@ozro.pw>
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

.function: a_function/1
    allocate_registers %2 local

    print (move %1 local %0 parameters) local
    izero %0 local
    return
.end

.function: main/1
    allocate_registers %3 local

    integer %1 local 42

    text %2 local "Hello World!"

    frame %1
    move @2 arguments %1 local
    call void a_function/1

    izero %0 local
    return
.end
