.function: [[entry_point]] main
    li $1, 0x00deadbeef000000u
    li $2, 0x000000deadbeef00u

    bitor $3, $1, $2

    ebreak
    return
.end
