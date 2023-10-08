.section ".text"

.symbol [[entry_point]] main
.label main
    float $1, 3.14
    li $2, 3u

    cmp $3, $1, $2

    ebreak
    return
