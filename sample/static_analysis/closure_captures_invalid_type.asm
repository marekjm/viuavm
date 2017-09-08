;
;   Copyright (C) 2017 Marek Marecki <marekjm@ozro.pw>
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
    iinc %2 local
    print %2 local
    return
.end

.function: main/0
    closure %1 local a_closure/0
    text %2 local "Hello World!"

    capturemove %1 local %2 %2 local

    frame %0
    call void %1 local

    izero %0 local
    return
.end
