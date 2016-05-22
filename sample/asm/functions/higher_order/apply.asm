.function: square/1
    ; this function takes single integer as its argument,
    ; squares it and returns the result
    imul 0 (arg 1 0) 1
    return
.end

.function: apply/2
    ; this function applies another function on a single parameter
    ;
    ; this function is type agnostic
    ; it just passes the parameter without additional processing
    .name: 1 func
    .name: 2 parameter

    ; apply the function to the parameter...
    frame ^[(pamv 0 (arg parameter 1))]
    fcall 3 (arg func 0)

    ; ...and return the result
    move 0 3
    return
.end

.function: main
    ; applies function square/1(int) to 5 and
    ; prints the result
    istore 1 5
    function 2 square/1

    frame ^[(param 0 2) (param 1 1)]
    print (call 3 apply/2)

    izero 0
    return
.end
