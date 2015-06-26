.def: check 1
    arg 0 1

    istore 2 2

    .mark: loop_begin
    ilt 1 2 3
    branch 3 loop_end loop_body

    .mark: loop_body
    isub 1 2
    jump loop_begin

    .mark: loop_end

    move 1 0
    not 0

    end
.end

.def: filter 1
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

    not 8
    branch 8 next_iter
    vpush 3 7

    .mark: next_iter

    empty 7
    iinc 4
    jump loop_begin

    .mark: loop_end

    move 3 0
    end
.end

.def: main 1
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

    function check 3

    frame 2
    param 0 3
    paref 1 1
    call filter 4

    print 4

    izero 0
    end
.end
