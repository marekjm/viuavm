.section ".text"

; A function to calculate an absolute value of its parameter.
; absz() takes any arithmetic value as its parameter, and outputs a signed
; integer.
.symbol absz
.label absz
.begin
    ; Establish whether the parameter is greater than zero. This will dictate
    ; whether we have to multiply it by -1 or 1 to get the absolute value.
    gt $2.l, $0.p, zero

    ; Turn the comparison result into a signed integer, so we can reuse it as
    ; the multiplier for absolutifing (this is not a real word, but you know
    ; what I mean) the parameter.
    add $2.l, zero, $2.l

    ; If the parameter is greater than zero we can jump straight to the
    ; epilogue. Otherwise we have to replace the multiplier with -1 to turn the
    ; non-positive value into a non-negative one.
    if $2.l, absz_epilogue
    li $2.l, -1

.label absz_epilogue
    ; Obtain the absolute value.
    ;
    ; This also ensures that the function produces the expected type by turning
    ; the parameter into a signed integer (since we have a signed integer in the
    ; left-hand side operand of the operation).
    mul.native $1.l, $2.l, $0.p

    return $1.l
.end

.symbol [[entry_point]] main
.label main
    ; Test on a negative, zero, and positive signed integer.
    frame $1.a
    li $0.a, -1
    call $1.l, absz

    frame $1.a
    li $0.a, 0
    call $2.l, absz

    frame $1.a
    li $0.a, 1
    call $3.l, absz

    ; Test on an unsigned integer, a float, and a double. Not to test the
    ; algorithm, but just to see if the function will return the expected type.
    frame $1.a
    li $0.a, 1u
    call $4.l, absz

    frame $1.a
    float $0.a, -1.0
    call $5.l, absz

    frame $1.a
    double $0.a, -1.0
    call $6.l, absz

    ; This would break the function because the result would wrap and become
    ; negative again. A simple solution to this problem is to use mul.saturate
    ; instead of the native operation provided by the host platform.
    ; frame $1.a
    ; li $0.a, -9223372036854775808
    ; call $7.l, absz

    ebreak
    return
