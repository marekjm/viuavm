.section ".text"

.symbol [[entry_point]] main
.label main
    li $1.l, 42
    ebreak

    move $2.l, $1.l
    ebreak

    return
