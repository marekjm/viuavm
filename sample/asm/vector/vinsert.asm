.def: main
    .name: 2 hurr
    .name: 3 durr
    .name: 4 ima
    .name: 5 sheep

    strstore hurr "Hurr"
    strstore durr "durr"
    strstore ima "Im'a"
    strstore sheep "sheep!"

    vec 1

    vinsert 1 sheep
    vinsert 1 hurr
    vinsert 1 durr 1
    vinsert 1 ima 2

    ; this works OK
    ; this instruction is here for debugging - uncomment it to see the vector
    ;print 1

    .name: 6 len
    .name: 7 counter

    istore counter 0
    vlen 1 len

    .mark: loop
    ilt counter len 8
    branch 8 inside break
    .mark: inside
    vat 1 9 @counter
    print 9
    ; empty must be done or second print would fail with a segfault!
    ; VATed registers are references!
    empty 9
    iinc counter
    jump loop

    .mark: break

    ; see comment for first vector-print
    ; it is for debugging - this should work without segfault
    ;print 1

    izero 0
    end
.end
