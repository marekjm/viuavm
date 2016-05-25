.function: run_in_a_process/0
    print (receive 1)
    return
.end

.function: main/1
    frame 0
    process 1 run_in_a_process/0

    frame ^[(param 0 1)]
    msg 0 detach/1

    frame ^[(param 0 1) (param 1 (istore 2 40))]
    msg 0 setPriority/2

    frame ^[(param 0 1) (param 1 (strstore 2 "Hello message passing World!"))]
    msg 0 pass/2

    izero 0
    return
.end
