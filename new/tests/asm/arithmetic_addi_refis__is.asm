.function: [[entry_point]] main
    li $1, 39
    ref $2, $1
    addi *2, $1, 3

    ebreak
    return
.end
