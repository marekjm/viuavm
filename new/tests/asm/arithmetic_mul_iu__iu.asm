.section ".text"

.symbol [[entry_point]] main
.label main
    li $1, 2u
    li $2, 21u
    mul $3, $1, $2

    ebreak
    return void
