.section ".text"

.symbol [[entry_point]] main
.label main
    li $1, 19u
    li $2, 23u
    add $3, $1, $2

    ebreak
    return void
