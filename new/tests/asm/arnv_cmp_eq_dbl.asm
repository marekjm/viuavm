.section ".text"

.symbol [[entry_point]] main
.label main
    double $1.l, 0.0

    li $2.l, 0
    eq $3.l, $1.l, $2.l

    li $2.l, 42
    eq $4.l, $1.l, $2.l

    li $2.l, 0u
    eq $5.l, $1.l, $2.l

    li $2.l, 42u
    eq $6.l, $1.l, $2.l

    float $2.l, 0.0
    eq $7.l, $1.l, $2.l

    float $2.l, 3.14
    eq $8.l, $1.l, $2.l

    double $2.l, 0.0
    eq $9.l, $1.l, $2.l

    double $2.l, 3.14159
    eq $10.l, $1.l, $2.l

    ebreak
    return
