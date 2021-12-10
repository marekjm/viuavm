.function: [[entry_point]] main
    buffer $1

    li $2, 42
    buffer_push $1, $2

    li $2, 64
    buffer_push $1, $2

    buffer_size $2, $1

    ebreak
    return
.end
