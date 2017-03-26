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

.function: main/1
    new %1 Object
    new %2 Object

    ; print object to be used as attribute value
    ; before it is inserted
    print %2

    .name: 3 key
    strstore %key "foo"

    ; insert and remove
    ; just to test move semantics
    remove %4 (insert %1 %key %2) %key

    ; print object used as attribute value
    ; after it was removed 
    print %4

    izero %0 local
    return
.end
