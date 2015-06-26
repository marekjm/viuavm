.function: square
    ; this function takes single integer as its argument,
    ; squares it and returns the result
    arg 0 1
    imul 1 1 0
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
    arg 0 1
    arg 1 2

    vec 3

    izero 4
    vlen 2 5

    .mark: loop_begin
    igte 4 5 6
    branch 6 loop_end loop_body

    .mark: loop_body

    frame 1
    vat 2 7 @4
    param 0 7
    fcall 1 8

    vpush 3 8
    empty 7

    iinc 4
    jump loop_begin

    .mark: loop_end

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
