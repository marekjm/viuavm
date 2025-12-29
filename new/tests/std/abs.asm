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
