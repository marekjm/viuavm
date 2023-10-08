.section ".text"

.symbol [[entry_point]] main
.label main
    frame $0.a
    actor $1, "dummy::function"

    ebreak
    return

.symbol "dummy::function"
.label "dummy::function"
    li $1, 41
    ebreak

    addi $1, $1, 1
    ebreak

    return $1
