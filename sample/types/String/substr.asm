.function: main
    frame ^[(param 0 (strstore 1 "Hello, World!"))]
    print (msg 4 substr)

    frame ^[(param 0 (strstore 1 "Hello, World!")) (param 1 (istore 2 0)) (param 2 (istore 3 5))]
    print (msg 4 substr)

    frame ^[(param 0 (strstore 1 "Hello, World!")) (param 1 (istore 2 7))]
    print (msg 4 substr)

    izero 0
    end
.end
