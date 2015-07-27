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

.function: main
    ; applies function square/1(int) to 5 and
    ; prints the result
    istore 1 5
    function 2 square

    frame ^[(param 0 2) (paref 1 1)]
    print (call 3 apply)

    izero 0
    end
.end
