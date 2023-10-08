.section ".text"

.symbol [[entry_point]] main
.label main
    li $1, -42
    li $2, 5
    mod $3, $1, $2

    ebreak
    return
