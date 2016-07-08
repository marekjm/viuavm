.function: fibonacci/2
    .name: 1 current_value
    .name: 2 accumulator
    arg current_value 0
    arg accumulator 1

    branch current_value +1 fibonacci/2__finished

    iadd accumulator current_value

    frame ^[(pamv 0 (idec current_value)) (pamv 1 accumulator)]
    tailcall fibonacci/2

    .mark: fibonacci/2__finished
    move 0 accumulator
    return
.end

.function: fibonacci/1
    .name: 2 accumulator

    frame ^[(pamv 0 (arg 1 0)) (pamv 1 (izero accumulator))]
    call accumulator fibonacci/2

    move 0 accumulator
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
