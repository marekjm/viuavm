.def: main 1
    strstore 1 "sample vector program"
    print 1

    vec 2

    vlen 2 3
    print 3

    vpush 2 1

    vlen 2 3
    print 3

    vpop 2 4
    print 4

    vlen 2 3
    print 3

    vpush 2 4
    vat 2 5
    vlen 2 3
    print 3
    print 5

    izero 0
    end
.end
