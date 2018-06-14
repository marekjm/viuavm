;
;   Copyright (C) 2016, 2017 Marek Marecki
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

.function: main/0
    ptr (.name: %iota pointer) local (integer (.name: %iota o) local 0) local

    .name: %iota message
    if *pointer yeah nope

    .mark: yeah
    string %message local "Yeah"
    jump done

    .mark: nope
    string %message local "Nope"

    .mark: done
    print %message local

    izero %0 local
    return
.end
