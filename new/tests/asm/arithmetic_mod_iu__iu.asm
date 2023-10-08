.section ".text"

.symbol [[entry_point]] main
.label main
    li $1, 42u
    li $2, 9u
    mod $3, $1, $2

    ebreak
    return
