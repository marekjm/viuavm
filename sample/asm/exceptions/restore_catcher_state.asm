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

.function: tertiary/1
    arg %3 local %0
    integer %4 local [[maybe_unused]] 300
    throw %3 local
    return
.end

.function: secondary/1
    arg %2 local %0
    integer %4 local [[maybe_unused]] 200

    frame ^[(param %0 %2 local)] %5
    integer %4 local [[maybe_unused]] 250
    call tertiary/1

    integer %4 local 225
    return
.end

.function: main/1
    integer %4 local [[maybe_unused]] 50

    try
    catch "Integer" .block: handle_integer
        ; draw caught object into 2 register
        print (draw %2 local) local
        print %4 local
        leave
    .end
    enter .block: main_block
        integer %4 local [[maybe_unused]] 100

        frame ^[(param %0 (integer %1 local 42) local)] %5
        call secondary/1

        integer %2 local 41
        integer %4 local 125
        leave
    .end

    ; leave instructions lead here
    print %2 local
    print %4 local

    izero %0 local
    return
.end


; catch "<type>" <block>    - registers a catcher <block> for given <type>
; enter <block>             - tries executing given block after registered catchers have been registered
; leave                     - leaves active block (if any) and resumes execution on instruction after the one that caused
;                             the block to be entered
