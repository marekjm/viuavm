.section ".text"

.symbol [[entry_point]] main
.label main
    li $1, 84
    li $2, 2
    div $3, $1, $2

    ebreak
    return void
