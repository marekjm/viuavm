.section ".text"

.symbol [[entry_point]] main
.label main
    li $1.l, 0u
    li $2.l, 1u
    li $3.l, -1u
    li $4.l, 18446744073709551615u

    ebreak
    return
