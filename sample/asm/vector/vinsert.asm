.def: main 1
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


    .name: 6 len
    .name: 7 counter

    istore counter 0
    vlen 1 len

    .mark: loop
    ilt counter len 8
    branch 8 :inside :break
    .mark: inside
    vat 1 9 @counter
    print 9
    iinc counter
    jump :loop

    .mark: break

    izero 0
    end
.end
