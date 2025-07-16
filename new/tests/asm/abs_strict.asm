.section ".text"

; See the comments in the abs_loose.asm file.
; This file's abs function always produces signed results.
.symbol abs
.begin
.label abs
    move $1.l, $0.p
    li $2.l, 0

    ; By default, the multiplier is -1. The function uses it to turn negative
    ; values into positive ones.
    li $3.l, -1

    ; If the parameter is negative, the function can jump straight to the
    ; epilogue to multiply by $3.l and return.
    lt $2.l, $1.l, $2.l 
    if $2.l, epilogue

    ; Otherwise, the function has to ensure the multiplier would not change the
    ; positive value into a negative one. How? Simply: by making the multiplier
    ; be 1.
    mul $3.l, $3.l, $3.l

.label epilogue

    ; To force the type of the return value to be signed, the function ALWAYS
    ; multiplies a signed value (-1 or 1) by the parameter.
    mul $1.l, $3.l, $1.l
    return $1.l
.end

.symbol [[entry_point]] main
.label main
    li $1.l, -17
    frame $1.a
    copy $0.a, $1.l
    call $2.l, abs

    li $3.l, 18u
    frame $1.a
    copy $0.a, $3.l
    call $4.l, abs

    li $5.l, 0u
    frame $1.a
    copy $0.a, $5.l
    call $6.l, abs

    ebreak
    return void

