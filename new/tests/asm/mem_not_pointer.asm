.section ".text"

.symbol [[entry_point]] main
.label main
    li $1.l, 0u
    li $2.l, 42u
    sw $2.l, $1.l, 0
    ebreak
    return
