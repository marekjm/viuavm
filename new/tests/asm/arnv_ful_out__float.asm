.section ".text"

.symbol [[entry_point]] main
.label main
    float $1.l, 2.14
    li $2.l, 1
    add $3.l, $1.l, $2.l

    ebreak
    return
