.function: [[entry_point]] main
    li $1, 16u
    li $2, 0xffffffffu
    li $3, 16u

    frame $3.a
    copy $0.a, $1.l
    move $1.a, $2.l
    copy $2.a, $3.l
    call $4.l, memset

    li $2, 40u
    frame $3.a
    move $0.a, $2.l
    move $1.a, $3.l
    move $2.a, $1.l
    call $5.l, memcpy

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

.function: memcpy
    li $1, 0u

    eq $2, $1, $2.p
    if $2, 9

    add $3, $1.p, $1.l
    lb $2, $3, 0

    add $3, $0.p, $1.l
    sb $2, $3, 0

    addi $1, $1, 1u
    jump 1

    move $2.l, $0.p
    return $2.l
.end
