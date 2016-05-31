.signature: typesystem::typeof/1

.function: main/0
    strstore 1 "Hello World!"
    ptr 2 1
    delete 1

    import "typesystem"

    frame ^[(param 0 2)]
    call 3 typesystem::typeof/1

    print 3

    izero 0
    return
.end
