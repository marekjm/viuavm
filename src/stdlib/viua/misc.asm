.function: misc::boolean/1
    ; this function returns boolean value of its parameter
    not (not (arg 0 0))
    return
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

    ; obtain the parameter and
    ; insert it at the beginning of the vector
    vinsert 2 (arg 4 @0)

    jump loop_begin

    .mark: loop_end

    ; delete the counter and
    ; move params vector to 1st register
    delete 1
    move 1 2

    ; cleanup
    delete 0
    delete 3

    leave
.end

.function: std::misc::cycle/1
    ; executes at least N cycles
    ;
    .name: 1 counter
    arg counter 0

    istore 2 9
    isub counter counter 2
    istore 2 2
    idiv counter counter 2

    .name: 2 zero
    izero zero

    .mark: __loop_begin
    branch (ilte 3 counter zero) __loop_end +1
    idec counter
    jump __loop_begin
    .mark: __loop_end

    return
.end
