.section ".text"

; A function to calculate an absolute value of its parameter.
; absn() takes any arithmetic value as its parameter, and outputs an unsigned
; integer.
.symbol absn
.label absn
.begin
    ; Establish whether the parameter is greater than zero. This will dictate
    ; whether we have to multiply it by -1 or 1 to get the absolute value.
    gt $2.l, $0.p, zero

    ; If the parameter is greater than zero we can jump straight to the
    ; epilogue. Otherwise we have to replace the multiplier with -1 to turn the
    ; non-positive value into a non-negative one.
    if $2.l, absn_epilogue
    li $2.l, -1

.label absn_epilogue
    ; Obtain the absolute value.
    mul.native $1.l, $2.l, $0.p

    ; Convert the result to an unsigned integer.
    add $1.l, uzero, $1.l

    return $1.l
.end

.symbol [[entry_point]] main
.label main
    ; Test on a negative, zero, and positive signed integer.
    frame $1.a
    li $0.a, -1
    call $1.l, absn

    frame $1.a
    li $0.a, 0
    call $2.l, absn

    frame $1.a
    li $0.a, 1
    call $3.l, absn

    ; Test on a signed integer, a float, and a double. Not to test the
    ; algorithm, but just to see if the function will return the expected type.
    frame $1.a
    li $0.a, 1u
    call $4.l, absn

    frame $1.a
    float $0.a, -1.0
    call $5.l, absn

    frame $1.a
    double $0.a, -1.0
    call $6.l, absn

    ; While this does not work for absz(), it works perfectly fine for absn()
    ; for to a slightly larger positive capacity of unsigned integers.
    frame $1.a
    li $0.a, -9223372036854775808
    call $7.l, absn

    ebreak
    return
