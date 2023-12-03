.section ".text"

.symbol [[entry_point]] main
.label main
    li $1.l, -42
	float $2.l, 0.0
	add $2.l, $2.l, $1.l

    ebreak
    return
