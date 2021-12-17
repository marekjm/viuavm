.function: [[entry_point]] main
    li $1, 0x00deadbeef000000u

    bitnot $2, $1

    ebreak
    return
.end
