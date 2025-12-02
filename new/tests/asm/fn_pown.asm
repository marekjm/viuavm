.section ".text"


.symbol [[extern]] pown


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
