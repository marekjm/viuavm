.section ".text"


.symbol pown
.label pown
    if $1.p, pown_exp_nonzero
    if $0.p, pown_base_nonzero

    addi $0.l, $0.p, 1
    return $0.l

.label pown_base_nonzero
    div $0.l, $0.p, $0.p
    return $0.l

.label pown_exp_nonzero
    copy $0.l, $0.p
    li $1.l, 1u

.label pown_loop
    eq $2.l, $1.l, $1.p
    if $2.l, pown_epilogue

    mul $0.l, $0.l, $0.p
    addi $1.l, $1.l, 1u

    if void, pown_loop

.label pown_epilogue
    return $0.l


.symbol [[entry_point]] main
.label main
    frame $2.a
    li $0.a, -1
    li $1.a, 0
    call $1.l, pown

    frame $2.a
    li $0.a, -1
    li $1.a, 1
    call $2.l, pown

    frame $2.a
    li $0.a, -1
    li $1.a, 2
    call $3.l, pown

    frame $2.a
    li $0.a, -1
    li $1.a, 3
    call $4.l, pown

    frame $2.a
    li $0.a, 0
    li $1.a, 0
    call $5.l, pown

    frame $2.a
    li $0.a, 0
    li $1.a, 1
    call $6.l, pown

    ebreak

    return
