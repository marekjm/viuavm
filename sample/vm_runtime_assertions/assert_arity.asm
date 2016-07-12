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

.block: try_ArityException
    frame ^[(param 0 (strstore 1 "Hello, World!")) (param 1 (istore 2 3)) (param 2 (istore 3 4)) (param 3 (istore 4 5))]
    print (msg 4 substr/)
    leave
.end

.block: catch_ArityException
    print (pull 6)
    leave
.end

.function: main/1
    try
    catch "ArityException" catch_ArityException
    enter try_ArityException

    izero 0
    return
.end
