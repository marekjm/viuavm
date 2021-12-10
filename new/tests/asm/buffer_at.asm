.function: [[entry_point]] main
    buffer $1

    li $2, 42
    buffer_push $1, $2

    li $2, 64
    buffer_push $1, $2

    li $2, 0u
    buffer_at $3, $1, $2

    ebreak
    return
.end
