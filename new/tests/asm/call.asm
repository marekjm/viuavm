.function: [[entry_point]] main
    atom $1, hello_world
    ebreak

    frame $1.a
    move $0.a, $1.l
    call void, dummy

    return void
.end

.function: dummy
    ebreak
    return void
.end
