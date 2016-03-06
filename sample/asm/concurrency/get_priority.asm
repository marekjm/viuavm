.function: run_in_a_thread
    nop
    nop
    nop
    nop
    return
.end

.function: main
    frame 0
    process 1 run_in_a_thread

    frame ^[(param 0 1)]
    print (msg 2 getPriority)

    thjoin 0 1

    izero 0
    return
.end
