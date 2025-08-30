.section ".text"

.symbol [[entry_point]] main
.label main
    ; Let's start with a zero.
    li $0.l, 0b0

    ; Then, let's see if loading small negative and positive numbers works. The
    ; -1 is always an interesting case due to its bit pattern.
    li $1.l, -0b1
    li $2.l, 0b1

    ; As the final test, let's see if loading maximums works.
    li $3.l, 0b111111111111111111111111111111111111111111111111111111111111111
    li $4.l, -0b1000000000000000000000000000000000000000000000000000000000000000

    ebreak
    return
