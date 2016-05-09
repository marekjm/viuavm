;.signature: lib::foo/1

.function: main
    frame ^[(pamv 0 (strstore 1 "Hello World!"))]
    call lib::foo/1

    izero 0
    return
.end
