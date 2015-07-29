.function: square
    ; this function takes single integer as its argument,
    ; squares it and returns the result
    imul 0 (arg 1 0) 1
    end
.end

.function: apply
    ; this function applies another function on a single parameter
    ;
    ; this function is type agnostic
    ; it just passes the parameter without additional processing
    .name: 1 func
    .name: 2 parameter

    ; apply the function to the parameter...
    frame ^[(param 0 (arg parameter 1))]
    fcall 3 (arg func 0)

    ; ...and return the result
    move 0 3
    end
.end

.function: map
    ; this function takes two arguments:
    ;   * a function,
    ;   * a vector,
    ; then, it maps (i.e. calls) the given function on every element of given vector
    ; and returns a vector containing modified values.
    ; returned vector is a newly created one - this function does not modify vectors in place.
    arg 1 0
    arg 2 1

    ; new vector to store mapped values
    vec 3

    ; set initial counter value and
    ; loop termination variable
    izero 4
    vlen 5 2

    ; while (...) {
    .mark: loop_begin
    branch (igte 6 4 5) loop_end

    ; call supplied function on current element...
    frame ^[(param 0 (vat 7 2 @4))]
    ; ...and push result to new vector
    vpush 3 (fcall 8 1)

    ; empty the register, as vat instruction creates references
    empty 7

    ; increase loop counter and go back to the beginning
    ;     ++i;
    ; }
    iinc 4
    jump loop_begin

    .mark: loop_end

    ; move vector with mapped values to the return register
    move 0 3
    end
.end

.function: main
    ; applies function square/1(int) to 5 and
    ; prints the result

    vpush (vec 1) (istore 2 1)
    vpush 1 (istore 2 2)
    vpush 1 (istore 2 3)
    vpush 1 (istore 2 4)
    vpush 1 (istore 2 5)

    print 1

    frame ^[(param 0 (function 3 square)) (paref 1 1)]
    print (call 4 map)

    izero 0
    end
.end
