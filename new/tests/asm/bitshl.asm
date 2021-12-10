.function: [[entry_point]] main
    li $1, 0x00000000deadbeefu
    li $2, 32u

    bitshl $3, $1, $2

    ebreak
    return
.end
