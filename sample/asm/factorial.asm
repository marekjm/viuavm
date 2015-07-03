.function: factorial
    .name: 1 number
    .name: 2 result
    arg number 0
    arg result 1

    ; multiply "result" (reference) by "number" (copy)
    ; and store the resulting integer in "result"
    ; calculation is available outside of the local scope
    imul 2 2 1
    idec 1

    istore 3 0
    ; if counter is equal to zero
    ; finish "factorial" calls
    ieq 4 1 3
    branch 4 finish
    ; this frame must be the same as in "main"
    frame 2
    param 0 number
    ; result must still be a reference
    paref 1 result
    call factorial
    .mark: finish
    end
.end

.function: main
    .name: 1 number
    .name: 2 result
    ; store the number of which we want to calculate the factorial
    istore number 8
    ; store result (starts with 1)
    istore result 1

    ; create frame for two parameters
    frame 2
    ; first is a copy of the number
    param 0 number
    ; second is a reference to result register
    ; because we want to display it here, after calls to factorial are finished
    paref 1 result
    call factorial

    ; print result
    print result
    izero 0
    end
.end
