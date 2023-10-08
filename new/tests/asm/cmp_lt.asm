.section ".text"

.symbol [[entry_point]] main
.label main
    li $1, -1
    li $2, 0

    cmp $3, $1, $2

    ebreak
    return
