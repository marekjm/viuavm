.def: main
    arg 0 1

    .name: 2 len
    .name: 3 counter

    vlen 1 len
    istore counter 0

    .mark: loop
    ilt counter len 4
    branch 4 inside break
    .mark: inside
    vat 1 5 @counter
    print 5
    empty 5
    iinc counter
    jump loop

    .mark: break

    izero 0
    end
.end
