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

.block: handler
    print (draw %2)
    leave
.end

.block: throws_derived
    throw (new %1 Combined)
    leave
.end

.function: typesystem_setup/0
    register (class %1 BaseA)
    register (derive (class %1 DerivedA) BaseA)
    register (derive (class %1 MoreDerivedA) DerivedA)

    register (class %1 BaseB)
    register (derive (class %1 DerivedB) BaseB)
    register (derive (class %1 MoreDerivedB) DerivedB)

    class %1 Combined
    derive %1 MoreDerivedA
    derive %1 MoreDerivedB
    register %1

    return
.end

.function: main/1
    frame %0
    call void typesystem_setup/0

    try
    catch "BaseA" handler
    enter throws_derived

    izero %0 local
    return
.end
