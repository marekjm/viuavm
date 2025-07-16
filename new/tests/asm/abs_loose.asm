.section ".text"

; A simple function to calculate absolute value of its parameter.
; See also the abs_strict.asm file.
.symbol abs
.begin
.label abs
    ; First, the values necessary for the function to work are set up:
    ;
    ;  - the actual argument the function was given ie, the parameter
    move $1.l, $0.p
    ;  - and the 0 that the parameter will be compared to
    li $2.l, 0

    ; Second, the parameter is compared to zero. You can think of the gt
    ; instruction as the > operator, and mentally translate the following line
    ; into:
    ;
    ;       $2.l = $1.l < $2.l
    ;
    ; which means, since we follow the usual rules of mathematics and logic:
    ;
    ;   Let $2.l store the value that is the result of comparison of value
    ;   stored in $1.l with the value stored in $2.l.
    gt $2.l, $1.l, $2.l 

    ; If the above comparison was true ie, the parameter is greater than zero,
    ; there is nothing for the function to do, so it can jump right to the
    ; epilogue.
    if $2.l, epilogue

    ; Otherwise, the parameter must be multiplied by -1 to produce a positive
    ; value. (Not really, because the function also handles 0 in this branch,
    ; but 0 multiplied by anything will give a 0 so it is fine.)
    li $2.l, -1
    mul $1.l, $2.l, $1.l

.label epilogue

    ; The type of the return value of this function depends on the type and
    ; value of its parameter:
    ;
    ;   - signed parameters produce signed results
    ;   - unsigned 0 parameter produces signed result
    ;   - unsigned non-zero parameters produce unsigned results
    ;
    ; Signed producing signed and unsigned producing unsigned make sense, but
    ; why would unsigned zero produce signed result? Due to how arithmetic
    ; operations work in Viua: their result is always of the same type as their
    ; left-hand operand.
    ;
    ; Since 0 > 0 is false, the function tries to turn the parameter
    ; non-negative by multiplying it by -1... and -1 is always signed, so the
    ; result of the whole function is also signed.
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
