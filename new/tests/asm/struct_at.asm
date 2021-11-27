.function: [[entry_point]] main
    struct $1
    atom $2, key
    li $3, 42

    struct_insert $1, $2, $3
    struct_at $3, $1, $2

    addi *3, *3, 1

    ebreak
    return void
.end
