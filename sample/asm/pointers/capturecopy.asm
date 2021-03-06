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

.closure: printer/0
    print %1 local
    return
.end

.function: main/0
    allocate_registers %4 local

    ptr (.name: %iota pointer) local (string (.name: %iota o) local "Hello World!") local

    closure (.name: %iota cl) local printer/0
    capturecopy %cl local %1 *pointer local

    frame %0
    call void %cl local

    izero %0 local
    return
.end
