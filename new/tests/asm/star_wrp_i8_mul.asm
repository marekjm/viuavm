.section ".text"

.symbol [[entry_point]] main
.label main
    ; Force the use of saturating arithmetic style for styled arithmetic
    ; operations.
    li $1.l, 0u
    earithmeticstyle void, $1.l

    ; Limit the arithmetic to 8 bits. Why? If the algoritmhs are correct they
    ; will work on any bit width, but using 8 bits means it is easier to write
    ; the tests as -128 is much easier to remember than whatever most negative
    ; 64-bit number is.
    li $1.l, 8u
    earithmeticwidth void, $1.l

    ;---------------------------------------------------------------------------
    ; MAIN BODY OF THE TEST
    ;----------------------


    ;-----------------------------------
    ; ZERO

    ; zero # zero
    li $1.l, 0
    li $2.l, 0
    stdmul $3.l, $1.l, $2.l

    ; zero # one
    li $1.l, 0
    li $2.l, 1
    stdmul $4.l, $1.l, $2.l

    ; zero # -one
    li $1.l, 0
    li $2.l, -1
    stdmul $5.l, $1.l, $2.l

    ; zero # max
    li $1.l, 0
    li $2.l, 127
    stdmul $6.l, $1.l, $2.l

    ; zero # min
    li $1.l, 0
    li $2.l, -128
    stdmul $7.l, $1.l, $2.l


    ;-----------------------------------
    ; ONE

    ; one # zero
    li $1.l, 1
    li $2.l, 0
    stdmul $8.l, $1.l, $2.l

    ; one # one
    li $1.l, 1
    li $2.l, 1
    stdmul $9.l, $1.l, $2.l

    ; one # -one
    li $1.l, 1
    li $2.l, -1
    stdmul $10.l, $1.l, $2.l

    ; one # max
    li $1.l, 1
    li $2.l, 127
    stdmul $11.l, $1.l, $2.l

    ; one # min
    li $1.l, 1
    li $2.l, -128
    stdmul $12.l, $1.l, $2.l


    ;-----------------------------------
    ; -ONE

    ; -one # zero
    li $1.l, -1
    li $2.l, 0
    stdmul $13.l, $1.l, $2.l

    ; -one # one
    li $1.l, -1
    li $2.l, 1
    stdmul $14.l, $1.l, $2.l

    ; -one # -one
    li $1.l, -1
    li $2.l, -1
    stdmul $15.l, $1.l, $2.l

    ; -one # max
    li $1.l, -1
    li $2.l, 127
    stdmul $16.l, $1.l, $2.l

    ; -one # min
    li $1.l, -1
    li $2.l, -128
    stdmul $17.l, $1.l, $2.l


    ;-----------------------------------
    ; MAX

    ; max # zero
    li $1.l, 127
    li $2.l, 0
    stdmul $18.l, $1.l, $2.l

    ; max # one
    li $1.l, 127
    li $2.l, 1
    stdmul $19.l, $1.l, $2.l

    ; max # -one
    li $1.l, 127
    li $2.l, -1
    stdmul $20.l, $1.l, $2.l

    ; max # max
    li $1.l, 127
    li $2.l, 127
    stdmul $21.l, $1.l, $2.l

    ; max # min
    li $1.l, 127
    li $2.l, -128
    stdmul $22.l, $1.l, $2.l


    ;-----------------------------------
    ; MIN

    ; min # zero
    li $1.l, -128
    li $2.l, 0
    stdmul $23.l, $1.l, $2.l

    ; min # one
    li $1.l, -128
    li $2.l, 1
    stdmul $24.l, $1.l, $2.l

    ; min # -one
    li $1.l, -128
    li $2.l, -1
    stdmul $25.l, $1.l, $2.l

    ; min # max
    li $1.l, -128
    li $2.l, 127
    stdmul $26.l, $1.l, $2.l

    ; min # min
    li $1.l, -128
    li $2.l, -128
    stdmul $27.l, $1.l, $2.l

    ebreak
    return
