.function: iterfib
    .name: 1 vector

    ress static
    isnull 2 vector
    not 2
    branch 2 logic
    vec vector
    istore 2 1
    vpush 1 2
    vpush 1 2

    .mark: logic

    .name: 3 number
    .name: 4 length
    arg number 0

    .mark: loop
    vlen length vector
    ilt 5 length number
    not 5
    branch 5 finished
    vat 6 vector -1
    vat 7 vector -2
    iadd 8 6 7
    vpush vector 8
    empty 6
    empty 7
    jump loop

    .mark: finished

    vat 9 vector -1
    copy 0 9
    empty 9
    end
.end

.function: main
    .name: 2 result
    .name: 3 expected
    istore expected 1134903170

    istore 1 45

    frame 1
    param 0 1
    call result iterfib

    print 2

    izero 0
    end
.end
