.function: [[entry_point]] main
    li $1.l, 2u
    amwa $1.l, $1.l, 0
    ebreak

    li $2.l, 0xaddeu
    sh $2.l, $1.l, 0

    li $2.l, 0xefbeu
    sh $2.l, $1.l, 1

    ebreak
    return
.end
