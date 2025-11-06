.section ".text"

.symbol [[entry_point]] main
.label main
    double $1.l, 2.01
    double $2.l, 0.0

    div $3.l, $1.l, $2.l
    div $4.l, $1.l, zero

    ebreak

    return void
