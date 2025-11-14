.section ".text"

.symbol [[entry_point]] main
.label main
    float $1.l, 2.0
    double $2.l, 2.0
    li $3.l, 2
    li $4.l, 2u

    sqrt $1.l, $1.l
    sqrt $2.l, $2.l
    sqrt $3.l, $3.l
    sqrt $4.l, $4.l
    ebreak

    return
