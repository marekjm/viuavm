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

.function: boolean/1
    move %0 local (not (not (arg %1 local %0 local) local) local) local
    return
.end

.function: main/1
    izero %1 local
    integer %2 local 1

    frame ^[(param %0 %1 local)]
    call %1 local boolean/1

    frame ^[(param %0 %2 local)]
    call %2 local boolean/1

    and %3 local %1 local %2 local

    print %1 local
    print %2 local
    print %3 local

    izero %0 local
    return
.end
