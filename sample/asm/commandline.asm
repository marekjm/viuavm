.function: main
    .name: 2 len
    .name: 3 counter

    vlen len (arg 1 0)
    istore counter 0

    .mark: loop
    branch (ilt 4 counter len) +1 break

    print (vat 5 1 @counter)
    empty 5

    iinc counter
    jump loop

    .mark: break

    izero 0
    return
.end
