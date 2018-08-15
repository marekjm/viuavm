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

.function: adder/1
    allocate_registers %2 local

    add %0 local (move %0 local %0 parameters) local (integer %1 local 21) local
    return
.end

.signature: std::functional::apply/2

.function: main/1
    allocate_registers %2 local

    import "std::functional"

    frame ^[(move %0 arguments (function %1 local adder/1) local) (move %1 arguments (integer %1 local 21) local)]
    call %1 local std::functional::apply/2
    print %1 local

    izero %0 local
    return
.end
