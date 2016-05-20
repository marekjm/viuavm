.function: main
    frame ^[(pamv 0 (strstore 1 "Hello World!"))]
    call foo/1

    izero 0
    return
.end
