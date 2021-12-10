.function: [[entry_point]] main
    li $1, 0xdeadbeef00000000u
    li $2, 32u

    bitshr $3, $1, $2

    ebreak
    return
.end
