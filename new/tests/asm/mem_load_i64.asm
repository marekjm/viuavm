.section ".text"

.symbol [[entry_point]] main
.label main
    li $1.l, 1u
    amda $1.l, $1.l, 0

    li $2.l, -1
    sd $2.l, $1.l, 0

    g.ld $3.l, $1.l, 0
    cast $3.l, int

    ebreak
    return
