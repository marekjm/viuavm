.section ".text"

.symbol [[entry_point]] main
.label main
    ; Force the use of wrapping arithmetic style for styled arithmetic
    ; operations.
    li $1.l, 0u
    earithmeticstyle void, $1.l

    ; Limit the arithmetic to 8 bits. Why? If the algoritmhs are correct they
    ; will work on any bit width, but using 8 bits means it is easier to write
    ; the tests as -128 is much easier to remember than whatever most negative
    ; 64-bit number is.
    li $1.l, 8u
    earithmeticwidth void, $1.l

    ; zero op zero
    li $1.l, 0
    li $2.l, 0
    stdadd $3.l, $1.l, $2.l

    ; -1 op 1
    li $1.l, -1
    li $2.l, 1
    stdadd $4.l, $1.l, $2.l

    ; -1 op -1
    li $1.l, -1
    li $2.l, -1
    stdadd $5.l, $1.l, $2.l

    ; 1 op 1
    li $1.l, 1
    li $2.l, 1
    stdadd $6.l, $1.l, $2.l

    ; negative limit op positive limit
    li $1.l, -128
    li $2.l, 127
    stdadd $7.l, $1.l, $2.l

    ; positive limit op negative limit
    li $1.l, 127
    li $2.l, -128
    stdadd $8.l, $1.l, $2.l

    ; negative limit op negative limit
    li $1.l, -128
    stdadd $9.l, $1.l, $1.l

    ; positive limit op positive limit
    li $1.l, 127
    stdadd $10.l, $1.l, $1.l

    ; negative limit op 1
    li $1.l, -128
    li $2.l, 1
    stdadd $11.l, $1.l, $2.l

    ; negative limit op -1
    li $1.l, -128
    li $2.l, -1
    stdadd $12.l, $1.l, $2.l

    ; positive limit op 1
    li $1.l, 127
    li $2.l, 1
    stdadd $13.l, $1.l, $2.l

    ; positive limit op -1
    li $1.l, 127
    li $2.l, -1
    stdadd $14.l, $1.l, $2.l

    ebreak
    return
