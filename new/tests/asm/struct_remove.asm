.function: [[entry_point]] main
    struct $1
    atom $2, key
    li $3, 42

    struct_insert $1, $2, $3
    struct_remove void, $1, $2

    ebreak
    return void
.end
