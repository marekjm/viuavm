.section ".text"


.symbol [[extern]] ln


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
;
; The formula used is "powers via logarithms":
;
;   a^b = e^[b * ln(a)]
;
; See the following links for more information:
;
;   https://en.wikipedia.org/wiki/Exponentiation#Powers_via_logarithms
;   https://stackoverflow.com/a/3519054
;
.symbol powr
.label powr
    ; this is ln(a)
    frame $1.a
    copy $0.a, $0.p
    call $1.l, ln

    ; this is x
    mul $2.l, $1.p, $1.l

    frame $1.a
    copy $0.a, $2.l
    call $0.l, exp
    return $0.l

; exp: a -> R
;   where
;       a: any arithmetic type
;       R: a real number
;
; The exp() function is basically e^x, but we do not have a built-in
; exponentiation operator.
; However, we can use the "power series" to calculate it using only simple
; arithmetic ie, multiplication and addition, like this:
;
;   e^x = 1 + x + (x^2 / 2!) + (x^3 / 3!) + ...
;
; The general formula is:
;
;         inf
;   e^x = SUM x^n / n!
;         n=0
;
; See https://en.wikipedia.org/wiki/Exponential_function#Power_series for more
; information.
.symbol exp
.label exp
    ; this is n
    li $1.l, 1

    ; this is the number of evaluations necessary to get a good enough result
    ; See comments about "number of evaluations" in the log.asm file.
    li $2.l, 49

    ; this is the accumulator for the whole function
    double $0.l, 1.0

    ; this is the cached result of exp_iter()
    double $3.l, 1.0

.label exp_loop
    frame $3.a
    copy $0.a, $3.l  ; previous
    copy $1.a, $0.p  ; x
    copy $2.a, $1.l  ; n
    call $3.l, exp_iter  ; previous = exp_iter(...)

    ; update the accumulator
    add $0.l, $0.l, $3.l

    ; update the loop counter, n
    addi $1.l, $1.l, 1

    lt $4.l, $1.l, $2.l
    if $4.l, exp_loop

    return $0.l

; To avoid overflow when calculating factorial we use a trick, and rewrite
;
;   exp_iter(x, n) = x^n / n!
;
; into
;
;   exp_iter(previous, x, n) = previous * x / n
;
; This requires the caller to store the previous result somewhere and supply for
; each evaluation of the exp_iter(), and that is the exp() function's job.
;
.symbol exp_iter
.label exp_iter
    ; this is tmp = (previous * x)
    mul $1.l, $0.p, $1.p

    ; this is (tmp / n)
    div $2.l, $1.l, $2.p

    return $2.l


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
