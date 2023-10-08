.section ".text"

.symbol [[entry_point]] main
.label main
    li $1, 3u
    float $2, 3.14

    cmp $3, $1, $2

    ebreak
    return
