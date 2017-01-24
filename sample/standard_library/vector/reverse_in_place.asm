;
;   Copyright (C) 2015, 2016 Marek Marecki
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

.signature: std::vector::of_ints/1
.signature: std::vector::reverse_in_place/1

.function: main/1
    link std::vector

    frame ^[(pamv %0 (istore %1 8))]
    call %1 std::vector::of_ints/1
    print %1

    frame ^[(param %0 %1)]
    call %2 std::vector::reverse_in_place/1
    print %2

    izero %0 local
    return
.end
