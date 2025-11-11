.section ".text"

.symbol [[entry_point]] main
.label main
    li $1.l, 0
    self $2.l

    eq void, $1.l, $2.l

    ebreak
    return
