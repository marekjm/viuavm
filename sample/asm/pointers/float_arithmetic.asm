;
;   Copyright (C) 2016, 2017, 2018 Marek Marecki
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

.function: main/0
    allocate_registers %6 local

    ptr (.name: %iota pointer_to_a) local (float (.name: %iota a) local 2.14) local
    ptr (.name: %iota pointer_to_b) local (float (.name: %iota b) local 1.41) local

    add (.name: %iota number) local *pointer_to_a local *pointer_to_b local
    print %number local

    izero %0 local
    return
.end
