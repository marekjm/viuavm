.section ".text"

.symbol [[entry_point]] main
.label main
	double $1.l, 6.02214076
	copy $2.l, $1.l
	cast $2.l, float

    ebreak
    return
