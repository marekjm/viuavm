.function: [[entry_point]] main
    string $1, "Hello, World!\n"
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
