.function: [[entry_point]] main
    frame $0.a
    call $1, dummy

    ebreak
    return void
.end

.function: dummy
    atom $1, hello_world
    return $1
.end
