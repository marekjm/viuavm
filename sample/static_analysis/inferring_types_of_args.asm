;
;   Copyright (C) 2017 Marek Marecki <marekjm@ozro.pw>
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

.function: main/1
    .name: iota an_arg

    arg %an_arg local %0    ; This will leave an_arg with undefined type.

    iinc %an_arg local      ; This will infer the type of an_arg to integer;
                            ; because iinc accepts only integers, an_arg must
                            ; be an integer for the program to be correct.
                            ; Since arg leaves types undefined the inferred type
                            ; does not conflict, and can be used as the new type
                            ; of the value in register an_arg.

    stof %iota local %an_arg local  ; Here, a type error will be reported.
                                    ; stof expects a text, but the type of an_arg was
                                    ; previously inferred to be integer, and
                                    ; these are two obviously conflicting assumptions.

    izero %0 local
    return
.end
