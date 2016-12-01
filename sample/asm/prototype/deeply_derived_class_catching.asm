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

.block: handler
    print (draw 2)
    leave
.end

.block: throws_derived
    throw (new 1 DeeplyDerived)
    leave
.end

.function: typesystem_setup/0
    register (class 1 Base)
    register (derive (class 1 Derived) Base)
    register (derive (class 1 MoreDerived) Derived)
    register (derive (class 1 EvenMoreDerived) MoreDerived)
    register (derive (class 1 DeeplyDerived) EvenMoreDerived)
    return
.end

.function: main/1
    call (frame 0) typesystem_setup/0

    try
    catch "Base" handler
    enter throws_derived

    izero 0
    return
.end
