.section ".text"

.symbol [[entry_point]] main
.label main
    li $1.l, 1u
    amqa $1.l, $1.l, 0

    self $2.l
    sq $2.l, $1.l, 0

    g.lq $3.l, $1.l, 0
    cast $3.l, pid

    ebreak
    return
