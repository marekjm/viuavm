.section ".text"

.symbol [[entry_point]] main
.label main
    li $1.l, 0u  ; false
    li $2.l, 1u  ; true

    and $3.l, $1.l, $1.l
    and $4.l, $1.l, $2.l
    and $5.l, $2.l, $1.l
    and $6.l, $2.l, $2.l

    ebreak

    return void
