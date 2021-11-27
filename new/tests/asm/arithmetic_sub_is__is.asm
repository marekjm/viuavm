.function: [[entry_point]] main
    li $1, 42
    li $2, 23
    sub $3, $1, $2

    ebreak
    return void
.end
