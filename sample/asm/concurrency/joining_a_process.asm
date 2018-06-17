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

.function: print_lazy/1
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    print (arg %1 local %0) local
    return
.end

.function: print_eager/1
    print (arg %1 local %0) local
    return
.end

.function: main/1
    frame ^[(param %0 (string %1 local "Hello concurrent World! (1)") local)]
    process %3 local print_lazy/1

    frame ^[(param %0 (string %2 local "Hello concurrent World! (2)") local)]
    process %4 local print_lazy/1

    join void %3 local
    join void %4 local

    izero %0 local
    return
.end
