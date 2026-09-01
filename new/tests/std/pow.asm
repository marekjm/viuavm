.section ".text"

;
; Exponentiation (power of) functions.
;

; pown: a -> N -> a
;   where
;       a: any arithmetic type
;       N: a natural (non-negative) integer
.symbol pown
.label pown
    if $1.p, pown_exp_nonzero
    if $0.p, pown_base_nonzero

    addi $0.l, $0.p, 1
    return $0.l

.label pown_base_nonzero
    div $0.l, $0.p, $0.p
    return $0.l

.label pown_exp_nonzero
    copy $0.l, $0.p
    li $1.l, 1u

.label pown_loop
    eq $2.l, $1.l, $1.p
    if $2.l, pown_epilogue

    mul $0.l, $0.l, $0.p
    addi $1.l, $1.l, 1u

    if void, pown_loop

.label pown_epilogue
    return $0.l


; powz: a -> Z -> R
;   where
;       a: any arithmetic type
;       Z: an integer
;       R: a real number
.symbol powz
.label powz
    copy $2.l, $1.p

    lt $1.l, $2.l, zero
    not $1.l, $1.l

    if $1.l, powz_call_pown
    muli $2.l, $2.l, -1

.label powz_call_pown
    frame $2.a
    copy $0.a, $0.p
    move $1.a, $2.l
    call $0.l, pown

    double $1.l, 1.0

    ; The result of pown() can be a zero, so we have to watch out.
    ; In case it actually is a zero, we should skip division.
    not $3.l, $0.l
    mul $0.l, $1.l, $0.l
    if $3.l, powz_epilogue

    div $0.l, $1.l, $0.l

.label powz_epilogue
    return $0.l


; powr: a -> R -> R
;   where
;       a: any arithmetic type
;       R: a real number
;
; In this case a "real number" means the highest precision floating point type
; available ie, the best approximation of a number belonging to the R set we can
; provide.
.symbol powr
.label powr
    return zero  ; FIXME not implemented


; factorial: a -> a
;   where
;       a: any arithmetic type
.symbol factorial
.label factorial
    lt $0.l, $0.p, zero
    if $0.l, factorial_of_negative

    eq $0.l, $0.p, zero
    if $0.l, factorial_of_zero

    ; this is the accumulator
    div $0.l, $0.p, $0.p

    ; this is n
    copy $1.l, $0.l

.label factorial_loop
    eq $2.l, $1.l, $0.p
    if $2.l, factorial_epilogue

    addi $1.l, $1.l, 1
    mul $0.l, $0.l, $1.l
    if void, factorial_loop

.label factorial_epilogue
    return $0.l

.label factorial_of_negative
    ; return a 0 of the matching type
    sub $0.l, $0.p, $0.p
    return $0.l

.label factorial_of_zero
    ; return a 1 of the matching type
    addi $0.l, $0.p, 1
    return $0.l
