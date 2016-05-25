.function: sum/4
    ; this function takes four integers as parameters and
    ; adds them, and returns the sum
    iadd 0 (arg 4 3) (iadd 0 (arg 3 2) (iadd 0 (arg 1 0) (arg 2 1)))
    return
.end

.function: invoke/2
    ; this function takes two parameters:
    ;    1) a function object
    ;    2) a vector of parameters for function given as first parameter
    ;
    ; it then creates a frame with required number of parameter slots (as
    ; specified by length of the vector), and calls given function with this
    ; frame
    arg 1 0
    arg 2 1

    ; take length of the vector
    .name: 4 vector_length
    vlen vector_length 2
    frame @vector_length

    ; zero loop counter
    .name: 3 loop_counter
    izero loop_counter
    .mark: while_begin

    ; simple condition:
    ; while (loop_counter < vector_length) {
    branch (igte 5 loop_counter vector_length) while_end while_body

    .mark: while_body

    .name: 7 slot
    ; store item located inside parameter vector at index denoted by loop_counter in
    ; a register and
    ; pass it as a parameter
    pamv @loop_counter (vat slot 2 @loop_counter)

    ; loop_counter++
    iinc loop_counter

    jump while_begin

    .mark: while_end

    ; finally, after the frame is ready
    ; call the function
    move 0 (fcall 8 1)
    return
.end

.function: main/1
    ; create the vector
    vpush (vec 1) (istore 2 20)
    vpush 1 (istore 3 16)
    vpush 1 (istore 4 8)
    vpush 1 (istore 5 -2)

    istore 2 20
    istore 3 16
    istore 4 8
    istore 5 -2

    ; call sum/4() function
    frame ^[(param 0 2) (param 1 3) (param 2 4) (param 3 5)]
    print (call 6 sum/4)

    ; call sum/4 function via invoke/2 function
    frame ^[(param 0 (function 7 sum/4)) (param 1 1)]
    print (call 8 invoke/2)

    izero 0
    return
.end
