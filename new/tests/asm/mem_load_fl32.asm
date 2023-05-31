.function: [[entry_point]] main
    li $1.l, 1u
    amwa $1.l, $1.l, 0

    float $2.l, 3.14159
    sw $2.l, $1.l, 0

    g.lw $3.l, $1.l, 0
    cast $3.l, float

    ebreak
    return
.end
