.section ".text"

.symbol factorial
.label factorial
    copy $0.l, $1.p
    li $1.l, 0

    eq $2.l, $0.l, $1.l
    if $2.l, factorial_epilogue

    lt $2.l, $0.l, $1.l
    if $2.l, factorial_epilogue

    copy $2.l, $1.p
.label factorial_loop
    subi $2.l, $2.l, 1

    ; See if the number reached zero, and if so break the loop.
    eq $3.l, $2.l, $1.l

    if $3.l, factorial_epilogue

    ; Otherwise, 
    mul $0.l, $0.l, $2.l
    if void, factorial_loop

.label factorial_epilogue
    return $0.l

.symbol [[entry_point]] main
.label main
    ; Valid call.
    li $1.l, 16
    frame $2.a
    copy $1.a, $1.l
    call $2.l, factorial

    ; Can't use a zero.
    li $3.l, 0
    frame $2.a
    copy $1.a, $3.l
    call $4.l, factorial

    ; Can't use a negative number.
    li $5.l, -1
    frame $2.a
    copy $1.a, $5.l
    call $6.l, factorial

    ebreak

    return void
