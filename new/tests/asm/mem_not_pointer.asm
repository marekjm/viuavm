.section ".text"

.symbol [[entry_point]] main
.label main
    li $1.l, 0u
    li $2.l, 42u

    ; This store is bad, because it tries to use something that is not a pointer
    ; to access memory. The value in $1.l is an integer, not a pointer.
    sw $2.l, $1.l, 0

    ebreak
    return
