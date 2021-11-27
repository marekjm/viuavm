.function: [[entry_point]] main
    li $1, 42u
    li $2, 23u
    sub $3, $1, $2

    ebreak
    return void
.end
