.section ".text"

.symbol [[entry_point]] main
.label main
    double $1.l, 2.14
    addi $2.l, $1.l, 1

    ebreak
    return
