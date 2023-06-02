.function: [[entry_point]] main
    atom $2.l, hello_world
    li $3.l, 42
    gts $2.l, $3.l

    frame $0.a
    call void, dummy

    return
.end

.function: dummy
    atom $2.l, hello_world
    gtl $3.l, $2.l

    ebreak
    return
.end
