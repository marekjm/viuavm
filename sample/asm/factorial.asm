.function: factorial/2
    .name: 1 number
    .name: 2 result
    imul result (arg result 1) (arg number 0)
    idec number

    ; if counter is equal to zero
    ; finish "factorial" calls
    branch (ieq 4 number (istore 3 0)) finish

    frame ^[(param 0 number) (param 1 result)]
    call result factorial/2

    .mark: finish
    move 0 result
    return
.end

.function: main/1
    .name: 1 number
    .name: 2 result
    ; store the number of which we want to calculate the factorial
    istore number 8
    ; store result (starts with 1)
    istore result 1

    ; create frame for two parameters:
    ; * first is a copy of the number
    ; * second is a reference to result register
    ;   because we want to display it here, after calls to factorial are finished
    frame ^[(param 0 number) (param 1 result)]
    call result factorial/2

    ; print result
    print result
    izero 0
    return
.end
