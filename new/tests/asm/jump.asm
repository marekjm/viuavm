.function: [[entry_point]] main
    atom $1, ok
    atom $2, not_ok

    jump 4
    move $1, $2

    ; 4th instruction
    noop

    ebreak
    return
.end
