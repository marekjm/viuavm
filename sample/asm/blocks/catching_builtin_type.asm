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

.block: handle_integer
    ; draw caught object into 2 register
    draw %2 local
    print %2 local
    leave
.end

.block: main_block
    integer %1 local 42
    throw %1 local
    leave
.end

.function: main/1
    try
    catch "Integer" handle_integer
    enter main_block
    ; leave instructions lead here

    izero %0 local
    return
.end


; catch "<type>" <block>    - registers a catcher <block> for given <type>
; enter <block>             - tries executing given block after registered catchers have been registered
; leave                     - leaves active block (if any) and resumes execution on instruction after the one that caused
;                             the block to be entered
