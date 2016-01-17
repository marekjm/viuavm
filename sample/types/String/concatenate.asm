.function: main
    frame ^[(param 0 (strstore 1 "Hello ")) (param 1 (strstore 2 "World!"))]
    msg 3 concatenate

    print 1
    print 2
    print 3

    izero 0
    return
.end
