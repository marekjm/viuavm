.function: [[entry_point]] main
    ; size of buffer
    li $3, 16u

    ; byte to use to fill the buffer
    li $2, 0xffu

    ; allocate space for first copy
    copy $1, $3
    amba $1.l, $1.l, 0

    ; initialise first copy
    ; memset(3): (void*, int, size_t)
    frame $3.a
    copy $0.a, $1.l
    move $1.a, $2.l
    copy $2.a, $3.l
    call $4.l, memset

    ; allocate space for second copy
    li $2, 16u
    amba $2, $2, 0

    ; make the copy
    ; memcpy(3): (void* dst, void* src, size_t)
    frame $3.a
    move $0.a, $2.l
    move $1.a, $1.l
    move $2.a, $3.l
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

    g.lt $2.l, $1.l, $2.p
    g.not $2.l, $2.l
    if $2.l, 10

    ; load from src
    add $3.l, $1.p, $1.l
    lb $2, $3, 0

    ; store to dst
    add $3, $0.p, $1.l
    sb $2, $3, 0

    ; increase counter and repeat
    addi $1, $1, 1u
    jump 1

    move $2.l, $0.p
    return $2.l
.end
