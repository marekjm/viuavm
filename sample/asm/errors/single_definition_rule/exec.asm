;.signature: lib::foo/1

.function: main/1
    frame ^[(pamv 0 (strstore 1 "Hello World!"))]
    call lib::foo/1

    izero 0
    return
.end
