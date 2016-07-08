.function: fibonacci/1
    izero 0
    return
.end

.function: main/0
    .name: 1 result

    frame ^[(pamv 0 (istore 1 20))]
    call result fibonacci/1

    print result

    izero 0
    return
.end
