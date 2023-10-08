.section ".text"

.symbol [[entry_point]] main
.label main
    li $1.l, 1u
    amda $1.l, $1.l, 0

    ; Avogadro constant
    double $2.l, 6.02214076
    sd $2.l, $1.l, 0

    g.ld $3.l, $1.l, 0
    cast $3.l, double

    ebreak
    return
