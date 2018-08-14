;
;   Copyright (C) 2017, 2018 Marek Marecki
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

.function: print_me/1
    allocate_registers %2 local

    move %1 local %0 parameters
    print *1 local

    return
.end

.function: main/0
    allocate_registers %3 local

    text %1 local "Hello World!"
    ptr %2 local %1 local

    frame ^[(move %0 arguments %2 local)]
    defer print_me/1

    izero %0 local
    return
.end
