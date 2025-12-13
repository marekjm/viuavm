.section ".text"

; A function to calculate an absolute value of its parameter.
; absr() takes any arithmetic value as its parameter, and outputs a double (a
; real number).
.symbol absr
.label absr
.begin
    ; Establish whether the parameter is less than zero. This will dictate
    ; whether we have to multiply it by -1 or 1 to get the absolute value.
    ;
    ; If you looked at the integer versions of abs() you would notice that the
    ; comparison here is "reversed". This is because floating point numbers have
    ; signed zeroes, and we want to have a non-negative zero as the result of
    ; absr()--I do not think it makes sense to return a negative zero as an
    ; absolute value.
    lt $2.l, $0.p, zero

    ; multiplier = ($0.p < 0) ? -1.0 : 1.0
    double $3.l, -1.0
    if $2.l, absr_epilogue
    double $3.l, 1.0

.label absr_epilogue
    ; Obtain the absolute value.
    ;
    ; This also ensures that the function produces the expected type by turning
    ; the parameter into a double (since we have a double in the left-hand side
    ; operand of the operation).
    mul $1.l, $3.l, $0.p

    return $1.l
.end

.symbol [[entry_point]] main
.label main
    ; Test on a negative, zero, and positive signed integer.
    frame $1.a
    li $0.a, -1
    call $1.l, absr

    frame $1.a
    li $0.a, 0
    call $2.l, absr

    frame $1.a
    li $0.a, 1
    call $3.l, absr

    ; Test on a signed integer, a float, and a double. Not to test the
    ; algorithm, but just to see if the function will return the expected type.
    frame $1.a
    li $0.a, 1u
    call $4.l, absr

    frame $1.a
    float $0.a, -1.41
    call $5.l, absr

    frame $1.a
    double $0.a, -3.14159
    call $6.l, absr

    ebreak
    return
