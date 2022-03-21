.function: [[entry_point]] main
    li $1, 1u
    ref $2, $1

    li $3, 42

    frame *2
    move $0.a, $3.l
    call void, dummy

    ebreak
    return
.end

.function: dummy
    ebreak
    return
.end
