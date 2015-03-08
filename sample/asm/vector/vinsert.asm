.def: main 1
    .name: 2 hurr
    .name: 3 durr
    .name: 4 ima
    .name: 5 sheep

    ;strstore hurr "Hurr"
    ;strstore durr "durr"
    ;strstore ima "Ima"
    ;strstore sheep "sheep"

    istore hurr 1
    istore durr 2
    istore ima 3
    istore sheep 4

    vec 1

    vpush 1 sheep
    vpush 1 sheep
    vpush 1 sheep
    vpush 1 sheep


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
