.function: [[entry_point]] main
    li $1, 42u
    li $2, 9u
    mod $3, $1, $2

    ebreak
    return
.end
