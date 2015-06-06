.def: square 1
    ; this function takes single integer as its argument,
    ; squares it and returns the result
    arg 0 1
    imul 1 1 0
    end
.end

.def: apply 1
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

.def: main 1
    ; applies function square/1(int) to 5 and
    ; prints the result
    istore 1 5
    function square 2

    frame 2
    param 0 2
    paref 1 1
    call apply 3

    print 3

    izero 0
    end
.end
