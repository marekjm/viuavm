.section ".text"

.symbol factorial_impl
.label factorial_impl
    li $1.l, 0

    eq $2.l, $1.p, $1.l
    if $2.l, factorial_epilogue

    lt $2.l, $1.p, $1.l
    if $2.l, factorial_epilogue

    mul $2.p, $2.p, $1.p

    subi $1.p, $1.p, 1

    ; Fake tail call.
    atxtp $2.l, @factorial_impl
    ; The noop is here just to prevent the disassembler for matching the
    ; atxtp+if pattern, and "demangling" it into a direct jump. In this case,
    ; the demangling would be mighty unhelpful because the factorial_impl is not
    ; a jump label and the reassembly would fail.
    noop
    if void, $2.l

.label factorial_epilogue
    return $2.p


.symbol factorial
.label factorial
    frame $3.a
    subi $1.a, $1.p, 1
    copy $2.a, $1.p
    call $1.l, factorial_impl

    return $1.l


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
