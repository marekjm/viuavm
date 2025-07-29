.section ".text"

.symbol [[entry_point]] main
.label main
    li $1.l, 0u  ; false
    li $2.l, 1u  ; true

    or $3.l, $1.l, $1.l
    or $4.l, $1.l, $2.l
    or $5.l, $2.l, $1.l
    or $6.l, $2.l, $2.l

    ebreak

    return void
