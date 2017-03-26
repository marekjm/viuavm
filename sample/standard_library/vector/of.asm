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

.signature: std::vector::of/2

.function: return_integer/1
    arg %0 %0
    return
.end

.function: main/1
    link std::vector

    register (class %1 Foo)

    frame ^[(pamv %0 (istore %1 8)) (pamv %1 (function %1 return_integer/1))]
    call %1 std::vector::of/2

    print %1

    izero %0 local
    return
.end
