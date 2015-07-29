.function: iterfib
    .name: 1 vector

    ress static
    branch (not (isnull 2 vector)) logic
    vpush (vpush (vec vector) (istore 2 1)) 2

    .mark: logic

    .name: 3 number
    .name: 4 length
    arg number 0

    .mark: loop
    branch (not (ilt 5 (vlen length vector) number)) finished
    iadd 8 (vat 6 vector -1) (vat 7 vector -2)
    vpush vector 8
    empty 6
    empty 7
    jump loop

    .mark: finished
    copy 0 (vat 9 vector -1)
    empty 9
    end
.end

.function: main
    .name: 2 result
    .name: 3 expected
    istore expected 1134903170

    frame ^[(param 0 (istore 1 45))]
    print (call result iterfib)

    izero 0
    end
.end
