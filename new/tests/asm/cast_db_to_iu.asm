.section ".text"

.symbol [[entry_point]] main
.label main
	double $1.l, 6.02214076
    li $2.l, 0u
    add $2.l, $2.l, $1.l

    ebreak
    return
