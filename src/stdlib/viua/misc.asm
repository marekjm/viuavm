.function: boolean
    ; this function returns boolean value of its parameter
    arg 0 0
    not 0
    not 0
    end
.end


.block: misc::argsvector
    ; this block obtains a vector of parameters passed to current function
    ;
    ; obtained vector is stored in register 1
    ;
    ; !!! WARNING !!! ACHTUNG !!! ATTENTION !!!
    ;
    ; code in this block needs 5 registers for operation.
    ; their indexes are currently hardcoded to be 0, 1, 2, 3 and 4.
    ; this registers MUST NOT be occupied when entering this block or
    ; user's data they contain will be overwritten and lost.
    ;
    ; when this block leaves, it cleans after itself and
    ; makes registers 0, 2, 4 and 4 available to user's code.

    ; vector for parameter storing
    vec 2

    ; loop counter and termination variable
    ; loop counter is the number of parameters current function has been called with
    argc 0
    izero 1

    .mark: loop_begin
    ; counting from N to zero
    ; when 0 is reached, stop iterating
    igt 3 0 1
    branch 3 loop_body loop_end

    .mark: loop_body

    ; decrement here, as parameter count must be converted to valid register index
    ; param count of 1 means last param index is 0
    idec 0

    ; obtain the parameter and...
    arg 4 @0
    ; ...insert it at the beginning of the vector
    vinsert 2 4
    ; free the leftover copy
    free 4

    jump loop_begin

    .mark: loop_end

    ; free the counter and
    ; move params vector to 1st register
    free 1
    move 1 2

    ; cleanup
    free 0
    free 3

    leave
.end
