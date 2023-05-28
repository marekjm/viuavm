.function: [[entry_point]] main
    li $1, 16u
    li $2, 0xffu
    li $3, 16u

    frame $3.a
    move $0.a, $1.l
    move $1.a, $2.l
    move $2.a, $3.l
    call $4.l, memset

    ebreak
    return
.end

.function: memset
    li $1, 0u

    eq $2.l, $1.l, $2.p
    if $2.l, 7

    add $3.l, $0.p, $1.l
    sb $1.p, $3.l, 0
    addi $1.l, $1.l, 1u
    jump 1

    move $4.l, $0.p
    return $4.l
.end
