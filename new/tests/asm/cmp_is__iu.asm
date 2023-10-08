.section ".text"

.symbol [[entry_point]] main
.label main
    li $1, 0
    li $2, 0u

    cmp $3, $1, $2

    ebreak
    return
