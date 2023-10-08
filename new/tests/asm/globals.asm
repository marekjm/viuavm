.section ".text"

.symbol [[entry_point]] main
.label main
    atom $2.l, hello_world
    li $3.l, 42
    gts $2.l, $3.l

    frame $0.a
    call void, dummy

    return

.symbol [[local]] dummy
.label dummy
    atom $2.l, hello_world
    gtl $3.l, $2.l

    ebreak
    return
