.function: is_divisible_by
    .name: 2 bound_variable
    arg 1 0

    .mark: loop_begin
    branch (ilt 3 1 bound_variable) loop_end loop_body

    .mark: loop_body
    isub 1 1 bound_variable
    jump loop_begin

    .mark: loop_end

    ; make zero "true" and
    ; non-zero values "false"
    ; FIXME: find out why `not (move 0 1)` causes memory leak
    not (copy 0 1)
    return
.end

.function: is_divisible_by_2
    closure 1 is_divisible_by
    enclose 1 2 (arg 2 0)
    move 0 1
    return
.end

.function: filter_closure
    ; classic filter() function
    ; it takes two arguments:
    ;   * a filtering function,
    ;   * a vector with values to be filtered,
    arg 1 0
    arg 2 1

    ; vector for filtered values
    vec 3

    ; initial loop counter and
    ; loop termination variable
    izero 4
    vlen 5 2

    ; while (...) {
    .mark: loop_begin
    branch (igte 6 4 5) loop_end

    ; call filtering function to determine whether current element
    ; is a valid value
    frame ^[(param 0 (vat 7 2 @4))] 0
    fcall 8 1

    ; if the result from filtering function was "true" - the element should be pushed onto result vector
    ; it it was "false" - skip to next iteration
    branch 8 element_ok next_iter

    .mark: element_ok
    vpush 3 7

    .mark: next_iter

    ; increase the counter and go back to the beginning of the loop
    ;     ++i;
    ; }
    iinc 4
    jump loop_begin

    .mark: loop_end

    ; move result vector into return register
    move 0 3
    return
.end

.function: main
    vpush (vec 1) (istore 2 1)
    vpush 1 (istore 2 2)
    vpush 1 (istore 2 3)
    vpush 1 (istore 2 4)
    vpush 1 (istore 2 5)

    print 1

    frame ^[(param 0 (istore 5 2))]
    call 3 is_divisible_by_2

    frame ^[(param 0 3) (param 1 1)]
    print (call 4 filter_closure)

    izero 0
    return
.end
