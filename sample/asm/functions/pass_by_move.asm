.function: foo/1
    move 0 (print (arg 1 0))
    return
.end

.function: main/1
    print (new 1 Object)

    frame ^[(pamv 0 1)]
    print (call 1 foo/1)

    izero 0
    return
.end
