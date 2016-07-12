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

.function: iterfib/1
    .name: 1 vector

    ress static
    branch (not (isnull 2 vector)) logic
    vpush (vpush (vec vector) (istore 2 1)) (istore 2 1)

    .mark: logic

    .name: 3 number
    .name: 4 length
    arg number 0

    .mark: loop
    branch (not (ilt 5 (vlen length vector) number)) finished
    iadd 8 (vat 6 vector -1) (vat 7 vector -2)
    vpush vector 8
    jump loop

    .mark: finished
    move 0 (vat 9 vector -1)
    return
.end

.function: main/1
    .name: 2 result
    .name: 3 expected
    istore expected 1134903170

    frame ^[(param 0 (istore 1 45))]
    print (call result iterfib/1)

    izero 0
    return
.end
