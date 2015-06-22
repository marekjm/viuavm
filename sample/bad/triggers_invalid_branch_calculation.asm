.def: sum 1
    ; sum integers in a vector
    izero 0

    arg 0 1

    izero 2
    vlen 1 3

    .mark: begin_loop
    igte 2 3 4

    branch 4 end_loop

    vat 1 4 @2
    iadd 0 4 0
    empty 4

    iinc 2
    jump begin_loop

    .mark: end_loop

    end
.end

.def: sum4 1
    ; this function will sum its 4 parameters
    arg 0 1
    arg 1 2
    arg 2 3
    arg 3 4

    iadd 1 2 0
    iadd 0 3 0
    iadd 0 4 0

    end
.end

.def: invoke 1
    arg 0 1
    arg 1 2

    strstore 3 "invoke("
    echo 3
    echo 1
    strstore 3 ", "
    echo 3
    echo 2
    strstore 3 ")"
    print 3

    free 3

    izero 3
    vlen 2 4

    .mark: while_begin
    igte 3 4 5
    branch 5 while_end while_body

    .mark: while_body

    ;vat 2 6 @3
    ;print 6
    ;empty 6

    print 3
    iinc 3

    jump while_begin

    .mark: while_end

    izero 0
    end
.end

.def: main 1
    ; create the vector
    vec 1

    istore 2 20
    vpush 1 2

    istore 3 16
    vpush 1 3

    istore 4 8
    vpush 1 4

    istore 5 -2
    vpush 1 5

    ; print
    print 1

    ; call ordinary sum/1() function
    frame 1
    paref 0 1
    call sum 6
    print 6

    ; call sum/4() function
    frame 4
    param 0 2
    param 1 3
    param 2 4
    param 3 5
    call sum4 6
    print 6

    function sum 7

    frame 2
    param 0 7
    param 1 1
    call invoke 8

    print 8

    izero 0
    end
.end
