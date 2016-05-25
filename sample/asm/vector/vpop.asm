.function: main/1
    print (vlen 3 (vec 2))

    vpush 2 (strstore 1 "Hello World!")
    print (vlen 3 2)

    vpop 4 2
    print (vlen 3 2)

    print 4

    izero 0
    return
.end
