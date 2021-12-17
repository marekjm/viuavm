.function: [[entry_point]] main
    atom $1, equal
    atom $2, not_equal

    eq $3, $1, $2

    ; 3rd instruction
    if $3, 6

    move $3, $2
    jump 7

    ; 6th instruction
    move $3, $1

    ; 7th instruction
    ebreak
    return
.end
