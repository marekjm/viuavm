.section ".text"

.symbol [[entry_point]] main
.label main
    li $1, 0x00deadbeef000000u

    bitnot $2, $1

    ebreak
    return
