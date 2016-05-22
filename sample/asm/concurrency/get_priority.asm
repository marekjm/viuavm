.function: run_in_a_process/0
    nop
    nop
    nop
    nop
    return
.end

.function: main
    frame 0
    process 1 run_in_a_process/0

    frame ^[(param 0 1)]
    print (msg 2 getPriority)

    join 0 1

    izero 0
    return
.end
