.function: [[entry_point]] main
    frame $0.a
    call $1, dummy

    ebreak
    return void
.end

.function: dummy
    string $1, "Hello, World!\n"
    return $1
.end
