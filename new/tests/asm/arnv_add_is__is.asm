.section ".text"

.symbol [[entry_point]] main
.label main
    li $1, 19
    li $2, 23
    add $3, $1, $2

    ebreak
    return void
