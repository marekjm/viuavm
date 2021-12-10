.function: [[entry_point]] main
    buffer $1
    li $2, 42
    buffer_push $1, $2

    ebreak
    return
.end
