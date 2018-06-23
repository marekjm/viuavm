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

.closure: a_closure/0
    print %2 local
    return
.end

.function: main/0
    allocate_registers %3 local

    closure %1 local a_closure/0
    integer %2 local 42

    capturemove %1 local %2 %2 local

    frame %0
    call void %1 local

    print %2 local

    izero %0 local
    return
.end
