.section ".text"

.symbol [[entry_point]] main
.label main
    li $1.l, 1u
    bitrev $1.l, $1.l

    li $2.l, 0xffffffff00000000u
    bitrev $2.l, $2.l

    li $3.l, 0x0000feedbeef0000u
    bitrev $3.l, $3.l

    li $4.l, 0x123456789abcdef0u
    bitrev $4.l, $4.l

    ebreak
    return
