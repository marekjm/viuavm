.signature: std::random::randint
.signature: std::io::getline

.block: check_the_number
    stoi 3 3
    leave
.end

.block: failed_to_convert
    echo (strstore 10 "guess: exception: ")
    print (pull 9)
    istore 3 -1
    leave
.end

.function: main
    ; implements this chapter of "The Rust Book": https://doc.rust-lang.org/book/guessing-game.html
    import "random"
    import "io"

    ; random integer between 1 and 100
    frame ^[(param 0 (istore 2 1)) (param 1 (istore 2 101))]
    call 1 std::random::randint

    ; enter zero to abort the game
    izero 0

    .mark: take_a_guess
    strstore 2 "guess the number: "
    echo 2

    frame 0
    call 3 std::io::getline

    tryframe
    catch "Exception" failed_to_convert
    try check_the_number

    branch (ieq 4 3 0) abort
    branch (ieq 4 3 1) correct

    branch (ilt 4 3 1) +1 +3
    strstore 4 "guess: your number is less than the target"
    jump incorrect
    strstore 4 "guess: your number is greater than the target"
    .mark: incorrect
    print 4
    jump take_a_guess

    .mark: correct
    print (strstore 2 "guess: correct")
    jump exit

    .mark: abort
    echo (strstore 2 "game aborted: target number was ")
    print 1

    .mark: exit
    izero 0
    end
.end
