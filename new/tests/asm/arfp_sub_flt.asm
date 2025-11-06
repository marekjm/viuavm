.section ".text"

.symbol [[entry_point]] main
.label main
    float $1.l, 2.01
    float $2.l, 1.13

    sub $3.l, $1.l, $2.l

    ebreak

    return void
