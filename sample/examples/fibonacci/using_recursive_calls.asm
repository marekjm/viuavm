.function: fibonacci/1
    .name: 1 current_value
    arg current_value 0

    branch current_value +1 fibonacci/0__finished

    frame ^[(pamv 0 (idec (copy 2 current_value)))]
    call 2 fibonacci/1

    iadd current_value 2

    .mark: fibonacci/0__finished
    move 0 current_value
    return
.end

.function: main/0
    .name: 1 result

    istore result 5

    frame ^[(pamv 0 result)]
    call result fibonacci/1

    print result

    izero 0
    return
.end
