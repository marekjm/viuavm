.section ".text"

.symbol [[entry_point]] main
.label main
    li $1, 0x00deadbeef000000u
    li $2, 0x000000deadbeef00u

    bitand $3, $1, $2

    ebreak
    return
