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

.function: is_not_negative/1
    allocate_registers %3 local

    gte %0 local (arg %1 local %0 local) local (izero %2 local) local
    return
.end

.signature: std::vector::every/2
.signature: std::vector::of_ints/1

.function: main/1
    allocate_registers %5 local

    import "std::vector"

    frame ^[(param %0 (integer %1 local 20) local)]
    call %2 local std::vector::of_ints/1

    vpush %2 local (integer %1 local -1) local

    frame ^[(param %0 %2 local) (move %1 arguments (function %3 local is_not_negative/1) local)]
    call %4 local std::vector::every/2
    print %4 local

    izero %0 local
    return
.end
