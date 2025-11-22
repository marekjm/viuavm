.section ".text"


; The factorial function is necessary since it is used as the denominator in
; exponentiation represented as a Taylor series.
; See https://en.wikipedia.org/wiki/Taylor_series#Exponential_function for more
; details.
.symbol factorial
.label factorial
    li $1.l, 2
    lt $2.l, $0.p, $1.l
    li $1.l, 1u
    if $2.l, factorial_epilogue

.label factorial_main
    copy $1.l, $0.p
    subi $2.l, $1.l, 1

.label factorial_loop
    mul $1.l, $1.l, $2.l
    subi $2.l, $2.l, 1
    if $2.l, factorial_loop

.label factorial_epilogue
    add $1.l, uzero, $1.l
    return $1.l


.symbol [[entry_point]] main
.label main
    frame $1.a
    li $0.a, 0
    call $1.l, factorial
    ebreak

    return
