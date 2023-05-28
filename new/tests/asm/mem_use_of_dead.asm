.function: [[entry_point]] main
    frame $0.a
    call $1.l, dummy
    ebreak

    lb $2.l, $1.l, 0
    ebreak

    return
.end

.function: dummy
    li $1.l, 0xffu

    li $2.l, 8u
    amba $2.l, $2.l, 0

    sb $1.l, $2.l, 0

    ebreak
    return $2.l
.end
