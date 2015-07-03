.function: square
    ; this function takes single integer as its argument,
    ; squares it and returns the result
    arg 0 1
    imul 0 1 1
    end
.end

.function: apply
    ; this function applies another function on a single parameter
    ;
    ; this function is type agnostic
    ; it just passes the parameter without additional processing
    .name: 1 func
    .name: 2 parameter

    ; extract the parameters
    arg 0 func
    arg 1 parameter

    ; apply the function to the parameter...
    frame 1
    param 0 parameter
    fcall func 3

    ; ...and return the result
    move 3 0
    end
.end

.function: map
    ; this function takes two arguments:
    ;   * a function,
    ;   * a vector,
    ; then, it maps (i.e. calls) the given function on every element of given vector
    ; and returns a vector containing modified values.
    ; returned vector is a newly created one - this function does not modify vectors in place.
    arg 0 1
    arg 1 2

    ; new vector to store mapped values
    vec 3

    ; set initial counter value and
    ; loop termination variable
    izero 4
    vlen 2 5

    ; while (...) {
    .mark: loop_begin
    igte 6 4 5
    branch 6 loop_end loop_body

    .mark: loop_body

    ; call supplied function on current element
    frame 1
    vat 7 2 @4
    param 0 7
    fcall 1 8

    ; push result to new vector
    vpush 3 8

    ; empty the register, as vat instruction creates references
    empty 7

    ; increase loop counter and go back to the beginning
    ;     ++i;
    ; }
    iinc 4
    jump loop_begin

    .mark: loop_end

    ; move vector with mapped values to the return register
    move 3 0
    end
.end

.function: main
    ; applies function square/1(int) to 5 and
    ; prints the result
    vec 1

    istore 2 1
    vpush 1 2
    istore 2 2
    vpush 1 2
    istore 2 3
    vpush 1 2
    istore 2 4
    vpush 1 2
    istore 2 5
    vpush 1 2

    print 1

    function square 3

    frame 2
    param 0 3
    paref 1 1
    call map 4

    print 4

    izero 0
    end
.end
