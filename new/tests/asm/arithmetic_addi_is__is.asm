.section ".text"

.symbol [[entry_point]] main
.label main
    li $1, 39
    addi $2, $1, 3

    ebreak
    return
