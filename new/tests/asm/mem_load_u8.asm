.function: [[entry_point]] main
    li $1.l, 1u
    amba $1.l, $1.l, 0

    li $2.l, 0xffu
    sb $2.l, $1.l, 0

    g.lb $3.l, $1.l, 0
    cast $3.l, uint

    ebreak
    return
.end
