.section ".text"

.symbol [[entry_point]] main
.label main
    li $1, 84u
    li $2, 2u
    div $3, $1, $2

    ebreak
    return void
