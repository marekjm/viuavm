.function: [[entry_point]] main
    frame $0.a
    actor $1, dummy

    ebreak
    return
.end

.function: dummy
    li $1, 41
    ebreak

    addi $1, $1, 1
    ebreak

    return $1
.end
