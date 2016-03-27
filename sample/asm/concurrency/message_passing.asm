.function: run_in_a_process
    print (receive 1)
    return
.end

.function: main
    frame 0
    process 1 run_in_a_process

    frame ^[(param 0 1)]
    msg 0 detach

    frame ^[(param 0 1) (param 1 (istore 2 40))]
    msg 0 setPriority

    frame ^[(param 0 1) (param 1 (strstore 2 "Hello message passing World!"))]
    msg 0 pass

    izero 0
    return
.end
