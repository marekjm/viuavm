.section ".text"

.symbol [[entry_point]] main
.label main
	li $1.l, -42
	li $2.l, 0u
	add $2.l, $2.l, $1.l

    ebreak
    return
