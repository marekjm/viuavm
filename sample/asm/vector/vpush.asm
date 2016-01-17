.function: main
    print (vlen 3 (vec 2))

    vpush 2 (strstore 1 "Hello World!")

    print (vlen 3 2)
    print (vpop 4 2)

    izero 0
    return
.end
