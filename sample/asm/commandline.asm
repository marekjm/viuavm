.function: main
    arg 0 1

    .name: 2 len
    .name: 3 counter

    vlen 1 len
    istore counter 0

    .mark: loop
    ilt 4 counter len
    branch 4 inside break
    .mark: inside
    vat 5 1 @counter
    print 5
    empty 5
    iinc counter
    jump loop

    .mark: break

    izero 0
    end
.end
