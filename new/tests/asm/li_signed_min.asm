.section ".text"

.symbol [[entry_point]] main
.label main
    li $1, -9223372036854775808

    ebreak
    return void
