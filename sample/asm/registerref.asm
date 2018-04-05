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

; This script's purpose is to stress register-reference syntax in
; integer related instructions.

.function: foo/0
    integer %1 local 16
    integer %16 local 1

    print %1 local
    print @1 local
    print %16 local
    print @16 local

    return
.end

.function: main/1
    frame %0 %17
    call void foo/0
    izero %0 local
    return
.end
