.section ".text"

.symbol [[entry_point]] main
.label main
	float $1.l, 3.14
    li $2.l, 0
    add $2.l, $2.l, $1.l

    ebreak
    return
