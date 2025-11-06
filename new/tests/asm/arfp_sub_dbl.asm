.section ".text"

.symbol [[entry_point]] main
.label main
    double $1.l, 2.01
    double $2.l, 1.13

    sub $3.l, $1.l, $2.l

    ebreak

    return void
