.function: [[extern]] "std::memset"

.function: [[entry_point]] main
    li $1, 16u
    amba $1.l, $1.l, 0

    li $2, 0xffu
    li $3, 16u

    frame $3.a
    move $0.a, $1.l
    move $1.a, $2.l
    move $2.a, $3.l
    call $4.l, "std::memset"

    ebreak
    return
.end
