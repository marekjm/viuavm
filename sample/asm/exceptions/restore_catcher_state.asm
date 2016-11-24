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



.function: tertiary/1
    arg 3 0
    istore (.unused: 4) 300
    throw 3
    return
.end

.function: secondary/1
    arg 2 0
    istore (.unused: 4) 200

    frame ^[(param 0 2)] 5
    istore 4 250
    call tertiary/1

    istore 4 225
    return
.end

.function: main/1
    istore 4 50

    try
    catch "Integer" .block: handle_integer
        ; pull caught object into 2 register
        print (pull 2)
        print 4
        leave
    .end
    enter .block: main_block
        istore 4 100

        frame ^[(param 0 (istore 1 42))] 5
        call secondary/1

        istore 2 41
        istore 4 125
        leave
    .end

    ; leave instructions lead here
    print 2
    print 4

    izero 0
    return
.end


; catch "<type>" <block>    - registers a catcher <block> for given <type>
; enter <block>             - tries executing given block after registered catchers have been registered
; leave                     - leaves active block (if any) and resumes execution on instruction after the one that caused
;                             the block to be entered
