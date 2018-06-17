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

.signature: std::random::randint
.signature: std::io::getline

.block: check_the_number
    stoi %3 local %3 local
    leave
.end

.block: failed_to_convert
    echo (string %10 local "guess: exception: ") local
    print (draw %9 local) local
    integer %3 local -1
    leave
.end

.function: main/1
    ; implements this chapter of "The Rust Book": https://doc.rust-lang.org/book/guessing-game.html
    import "random"
    import "io"

    ; random integer between 1 and 100
    frame ^[(param %0 (integer %2 local 1) local) (param %1 (integer %2 local 101) local)]
    call %1 local std::random::randint

    ; enter zero to abort the game
    izero %0 local

    .mark: take_a_guess
    string %2 local "guess the number: "
    echo %2 local

    frame %0
    call %3 local std::io::getline

    try
    catch "Exception" failed_to_convert
    enter check_the_number

    if (eq %4 local %3 local %0 local) local abort
    if (eq %4 local %3 local %1 local) local correct

    if (lt %4 local %3 local %1 local) local +1 +3
    string %4 local "guess: your number is less than the target"
    jump incorrect
    string %4 local "guess: your number is greater than the target"
    .mark: incorrect
    print %4 local
    jump take_a_guess

    .mark: correct
    print (string %2 local "guess: correct") local
    jump exit

    .mark: abort
    echo (string %2 local "game aborted: target number was ") local
    print %1 local

    .mark: exit
    izero %0 local
    return
.end
