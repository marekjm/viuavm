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

.signature: std::random::randint
.signature: std::io::getline

.block: check_the_number
    stoi %3 %3
    leave
.end

.block: failed_to_convert
    echo (strstore %10 "guess: exception: ")
    print (draw %9)
    istore %3 -1
    leave
.end

.function: main/1
    ; implements this chapter of "The Rust Book": https://doc.rust-lang.org/book/guessing-game.html
    import "random"
    import "io"

    ; random integer between 1 and 100
    frame ^[(param %0 (istore %2 1)) (param %1 (istore %2 101))]
    call %1 std::random::randint

    ; enter zero to abort the game
    izero %0

    .mark: take_a_guess
    strstore %2 "guess the number: "
    echo %2

    frame %0
    call %3 std::io::getline

    try
    catch "Exception" failed_to_convert
    enter check_the_number

    if (eq int64 %4 %3 %0) abort
    if (eq int64 %4 %3 %1) correct

    if (lt int64 %4 %3 %1) +1 +3
    strstore %4 "guess: your number is less than the target"
    jump incorrect
    strstore %4 "guess: your number is greater than the target"
    .mark: incorrect
    print %4
    jump take_a_guess

    .mark: correct
    print (strstore %2 "guess: correct")
    jump exit

    .mark: abort
    echo (strstore %2 "game aborted: target number was ")
    print %1

    .mark: exit
    izero %0
    return
.end
