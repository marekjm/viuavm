.function: foreach
    ; this function takes two mandatory parameters:
    ;
    ;       * a closure, or a function object to call on every element of second
    ;         parameter,
    ;       * a vector of values,
    ;
    ; third parameter is optional and
    ; is a vector that will store return values from callback function
    ; (NOTICE: thrid parameter is not implemented)
    ;
    .name: 1 callback
    .name: 2 list
    arg callback 0
    arg list 1

    .name: 3 counter
    izero counter
    vlen 4 list

    .mark: loop_begin
    ilt 5 3 4

    branch 5 loop_body loop_end

    .mark: loop_body

    vat 6 list @counter

    frame 1
    param 0 6
    fcall 7 callback

    empty 6

    iinc counter
    jump loop_begin

    .mark: loop_end

    end
.end
