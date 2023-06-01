.function: [[entry_point]] main
    li $1.l, 1u
    amha $1.l, $1.l, 0

    li $2.l, -1
    sh $2.l, $1.l, 0

    g.lh $3.l, $1.l, 0
    cast $3.l, int

    ebreak
    return
.end
