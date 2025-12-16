.section ".text"


.symbol [[extern]] powz


.symbol [[entry_point]] main
.label main
    ; Same tests as for pown(), because powz() should be compatible with pown()
    ; for the same set of arguments (with the caveat that powz returns a
    ; double).
    frame $2.a
    li $0.a, -1
    li $1.a, 0
    call $1.l, powz

    frame $2.a
    li $0.a, -1
    li $1.a, 1
    call $2.l, powz

    frame $2.a
    li $0.a, -1
    li $1.a, 2
    call $3.l, powz

    frame $2.a
    li $0.a, -1
    li $1.a, 3
    call $4.l, powz

    frame $2.a
    li $0.a, 0
    li $1.a, 0
    call $5.l, powz

    frame $2.a
    li $0.a, 0
    li $1.a, 1
    call $6.l, powz

    ; Additional tests for powz().
    frame $2.a
    li $0.a, 2
    li $1.a, -1
    call $7.l, powz

    frame $2.a
    li $0.a, 4
    li $1.a, -2
    call $8.l, powz

    frame $2.a
    li $0.a, 0
    li $1.a, -1
    call $9.l, powz

    ebreak

    return
