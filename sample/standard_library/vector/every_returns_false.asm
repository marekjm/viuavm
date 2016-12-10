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

.function: is_not_negative/1
    gte int64 0 (arg 1 0) (izero 2)
    return
.end

.signature: std::vector::every/2
.signature: std::vector::of_ints/1

.function: main/1
    link std::vector

    frame ^[(param 0 (istore 1 20))]
    call 2 std::vector::of_ints/1

    vpush 2 (istore 1 -1)

    frame ^[(param 0 2) (pamv 1 (function 4 is_not_negative/1))]
    call 5 std::vector::every/2
    print 5

    izero 0
    return
.end
