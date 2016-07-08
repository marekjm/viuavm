.function: fibonacci/2
    .name: 1 current_value
    .name: 2 accumulator
    arg current_value 0
    arg accumulator 1

    branch current_value +1 fibonacci/0__finished

    iadd accumulator current_value

    frame ^[(pamv 0 (idec current_value)) (pamv 1 accumulator)]
    tailcall fibonacci/2

    .mark: fibonacci/0__finished
    move 0 accumulator
    return
.end

.function: main/0
    .name: 1 result

    istore result 5

    ; pass accumulator in second parameter slot
    frame ^[(pamv 0 result) (pamv 1 (izero 2)]
    call result fibonacci/2

    print result

    izero 0
    return
.end
