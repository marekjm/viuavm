.section ".text"

.symbol [[entry_point]] main
.label main
    li $1.l, -42
    copy $2.l, $1.l
    cast $2.l, float

    ebreak
    return
