;
;   Copyright (C) 2017, 2018 Marek Marecki
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
    allocate_registers %5 local

    text (.name: %iota hello_world) local "Hello World!"
    print %hello_world local

    integer (.name: %iota first_index) local 0
    integer (.name: %iota last_index) local 6

    textsub (.name: %iota hello) local %hello_world local %first_index local %last_index local
    print %hello local

    izero %0 local
    return
.end
