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

.signature: std::vector::of/2

.function: return_integer/1
    allocate_registers %1 local

    arg %0 local %0
    return
.end

.function: main/1
    allocate_registers %2 local

    import "std::vector"

    frame ^[(move %0 arguments (integer %1 local 8) local) (move %1 arguments (function %1 local return_integer/1) local)]
    call %1 local std::vector::of/2

    print %1 local

    izero %0 local
    return
.end
