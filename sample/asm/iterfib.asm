.function: iterfib/1
    .name: 1 vector

    ress static
    branch (not (isnull 2 vector)) logic
    vpush (vpush (vec vector) (istore 2 1)) (istore 2 1)

    .mark: logic

    .name: 3 number
    .name: 4 length
    arg number 0

    .mark: loop
    branch (not (ilt 5 (vlen length vector) number)) finished
    iadd 8 (vat 6 vector -1) (vat 7 vector -2)
    vpush vector 8
    jump loop

    .mark: finished
    move 0 (vat 9 vector -1)
    return
.end

.function: main/1
    .name: 2 result
    .name: 3 expected
    istore expected 1134903170

    frame ^[(param 0 (istore 1 45))]
    print (call result iterfib/1)

    izero 0
    return
.end
