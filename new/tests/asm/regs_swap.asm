.section ".text"

.symbol [[entry_point]] main
.label main
    li $1.l, 42
    li $2.l, 666
    ebreak

    swap $2.l, $1.l
    ebreak

    return
