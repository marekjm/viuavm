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
