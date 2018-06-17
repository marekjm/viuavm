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

.signature: test_module::test_fun/0

.block: try_calling
    frame %0
    call test_module::test_fun/0
    leave
.end
.block: handle_Integer
    draw %1 local
    echo (string %2 local "looks ") local

    if %1 local +2
    string %2 local "truthy"
    string %2 local "falsey"

    echo %2 local
    echo (string %2 local ": ") local
    print %1 local

    leave
.end

.function: main/0
    import "test_module"

    try
    catch "Integer" handle_Integer
    enter try_calling

    izero %0 local
    return
.end
