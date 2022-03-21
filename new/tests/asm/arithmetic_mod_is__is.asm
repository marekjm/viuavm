.function: [[entry_point]] main
    li $1, -42
    li $2, 5
    mod $3, $1, $2

    ebreak
    return
.end
