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
    ; this function takes two parameters:
    ;   1)  a function
    ;   2)  a vector containing parameters for function passed as first parameter
    ;
    ; it then unpacks the params in vector and
    ; calls given function with them as params
    .name: 0 func
    .name: 1 params

    arg 0 func
    arg 1 params

    vlen 1 2
    frame @2

    strstore 8 "index: "
    strstore 9 "continue: "
    strstore 10 "object: "

    izero 3
    .mark: begin_loop
    igte 3 2 4

    ;echo 9
    print 4
    branch 4 end_loop

    ;vat params 4 @3
    ;echo 3
    ;print 3
    ;echo 10
    ;print 4
    ;param @3 4
    ;empty 4

    ;iinc 3
    ;jump begin_loop

    .mark: end_loop

    ;call sum 1

    ; these NOP instructions are here as a padding
    ; probably there is a bug in the assembler or bytecode generator as
    ; without them, the "izero 1" seems to not be executed
    nop
    nop
    izero 1
    move 1 0
    end
.end

.def: main 1
    vec 1

    istore 2 20
    vpush 1 2

    istore 3 16
    vpush 1 3

    istore 4 8
    vpush 1 4

    istore 5 -2
    vpush 1 5

    print 1

    frame 1
    paref 0 1
    call sum 6
    print 6

    frame 4
    param 0 2
    param 1 3
    param 2 4
    param 3 5
    call sum4 6
    print 6

    function sum 7
    frame 2 16
    param 0 7
    param 1 1
    call invoke 6
    print 6

    izero 0
    end
.end
