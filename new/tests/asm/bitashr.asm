.section ".text"

.symbol [[entry_point]] main
.label main
    li $1, 0xfeedbeef00000000u
    li $2, 32u

    bitashr $3, $1, $2

    ebreak
    return
