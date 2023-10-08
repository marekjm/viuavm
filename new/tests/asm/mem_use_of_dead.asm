.section ".text"

.symbol [[entry_point]] main
.label main
    frame $0.a
    call $1.l, dummy
    ebreak

    ; This LB will crash the program, because it uses memory allocated by the
    ; dummy() function as its local area.
    lb $2.l, $1.l, 0
    ebreak

    return

.symbol dummy
.label dummy
    li $1.l, 0xffu

    li $2.l, 8u
    amba $2.l, $2.l, 0

    sb $1.l, $2.l, 0

    ebreak
    return $2.l
