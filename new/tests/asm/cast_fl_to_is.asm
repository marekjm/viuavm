.section ".text"

.symbol [[entry_point]] main
.label main
	float $1.l, 3.14
	copy $2.l, $1.l
	cast $2.l, int

    ebreak
    return
