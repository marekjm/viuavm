.function: main
    frame ^[(param 0 (strstore 1 "Hello World!")) (param 1 (strstore 2 "World!"))]
    print (msg 3 endswith/2)

    frame ^[(param 0 1) (param 1 (strstore 2 "Fail"))]
    print (msg 3 endswith/2)

    izero 0
    return
.end
