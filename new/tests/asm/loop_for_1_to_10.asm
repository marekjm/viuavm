.function: [[entry_point]] main
    li $1, 1u
    li $2, 10u

    ; 2nd instruction
    gt $3, $1, $2
    if $3, 7

    ebreak
    addi $1, $1, 1u
    jump 2

    ; 7th instruction
    ebreak
    return
.end
