.function: main
    vpush (vpush (vec 1) (strstore 2 "formatted")) (strstore 2 "World")

    frame ^[(param 0 (strstore 2 "Hello, #{0} #{1}!")) (param 1 1)]
    print (msg 3 format/)

    izero 0
    return
.end
