.section ".text"

.symbol [[extern]] factorial

.symbol [[entry_point]] main
.label main
    frame $1.a
    li $0.a, 0
    call $0.l, factorial

    frame $1.a
    li $0.a, 1
    call $1.l, factorial

    frame $1.a
    li $0.a, 2
    call $2.l, factorial

    frame $1.a
    li $0.a, 5
    call $5.l, factorial

    ; Can't use a negative number.
    frame $1.a
    li $0.a, -1
    call $6.l, factorial

    ebreak
    return void
