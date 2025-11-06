.section ".text"

.symbol [[entry_point]] main
.label main
    float $1.l, 2.01
    float $2.l, 0.0

    div $3.l, $1.l, $2.l
    div $4.l, $1.l, zero

    ebreak

    return void
