.def: iterfib
    .name: 1 vector

    ress static
    isnull vector 2
    not 2
    branch 2 logic
    vec vector
    istore 2 1
    vpush 1 2
    vpush 1 2

    .mark: logic

    .name: 3 number
    .name: 4 length
    arg 0 number

    .mark: loop
    vlen vector length
    ilt length number 5
    not 5
    branch 5 finished
    vat vector 6 -1
    vat vector 7 -2
    iadd 6 7 8
    vpush vector 8
    empty 6
    empty 7
    jump loop

    .mark: finished

    vat vector 9 -1
    copy 9 0
    empty 9
    end
.end

.def: main
    .name: 2 result
    .name: 3 expected
    istore expected 1134903170

    istore 1 45

    frame 1
    param 0 1
    call iterfib result

    print 2

    izero 0
    end
.end
