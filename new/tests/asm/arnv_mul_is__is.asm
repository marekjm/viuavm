.section ".text"

.symbol [[entry_point]] main
.label main
    li $1, 21
    li $2, 2
    mul $3, $1, $2

    ebreak
    return void
