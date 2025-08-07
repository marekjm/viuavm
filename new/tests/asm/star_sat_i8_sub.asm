.section ".text"

.symbol [[entry_point]] main
.label main
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
    stdsub.saturate $3.l, $1.l, $2.l

    ; zero # one
    li $1.l, 0
    li $2.l, 1
    stdsub.saturate $4.l, $1.l, $2.l

    ; zero # -one
    li $1.l, 0
    li $2.l, -1
    stdsub.saturate $5.l, $1.l, $2.l

    ; zero # max
    li $1.l, 0
    li $2.l, 127
    stdsub.saturate $6.l, $1.l, $2.l

    ; zero # min
    li $1.l, 0
    li $2.l, -128
    stdsub.saturate $7.l, $1.l, $2.l


    ;-----------------------------------
    ; ONE

    ; one # zero
    li $1.l, 1
    li $2.l, 0
    stdsub.saturate $8.l, $1.l, $2.l

    ; one # one
    li $1.l, 1
    li $2.l, 1
    stdsub.saturate $9.l, $1.l, $2.l

    ; one # -one
    li $1.l, 1
    li $2.l, -1
    stdsub.saturate $10.l, $1.l, $2.l

    ; one # max
    li $1.l, 1
    li $2.l, 127
    stdsub.saturate $11.l, $1.l, $2.l

    ; one # min
    li $1.l, 1
    li $2.l, -128
    stdsub.saturate $12.l, $1.l, $2.l


    ;-----------------------------------
    ; -ONE

    ; -one # zero
    li $1.l, -1
    li $2.l, 0
    stdsub.saturate $13.l, $1.l, $2.l

    ; -one # one
    li $1.l, -1
    li $2.l, 1
    stdsub.saturate $14.l, $1.l, $2.l

    ; -one # -one
    li $1.l, -1
    li $2.l, -1
    stdsub.saturate $15.l, $1.l, $2.l

    ; -one # max
    li $1.l, -1
    li $2.l, 127
    stdsub.saturate $16.l, $1.l, $2.l

    ; -one # min
    li $1.l, -1
    li $2.l, -128
    stdsub.saturate $17.l, $1.l, $2.l


    ;-----------------------------------
    ; MAX

    ; max # zero
    li $1.l, 127
    li $2.l, 0
    stdsub.saturate $18.l, $1.l, $2.l

    ; max # one
    li $1.l, 127
    li $2.l, 1
    stdsub.saturate $19.l, $1.l, $2.l

    ; max # -one
    li $1.l, 127
    li $2.l, -1
    stdsub.saturate $20.l, $1.l, $2.l

    ; max # max
    li $1.l, 127
    li $2.l, 127
    stdsub.saturate $21.l, $1.l, $2.l

    ; max # min
    li $1.l, 127
    li $2.l, -128
    stdsub.saturate $22.l, $1.l, $2.l


    ;-----------------------------------
    ; MIN

    ; min # zero
    li $1.l, -128
    li $2.l, 0
    stdsub.saturate $23.l, $1.l, $2.l

    ; min # one
    li $1.l, -128
    li $2.l, 1
    stdsub.saturate $24.l, $1.l, $2.l

    ; min # -one
    li $1.l, -128
    li $2.l, -1
    stdsub.saturate $25.l, $1.l, $2.l

    ; min # max
    li $1.l, -128
    li $2.l, 127
    stdsub.saturate $26.l, $1.l, $2.l

    ; min # min
    li $1.l, -128
    li $2.l, -128
    stdsub.saturate $27.l, $1.l, $2.l

    ebreak
    return
