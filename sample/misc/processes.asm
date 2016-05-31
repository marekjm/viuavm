.function: run_in_a_process/1
    echo (strstore 1 "spawned process ")
    echo (arg 1 0)
    print (strstore 1 " exiting")
    return
.end

.function: process_spawner/1
    echo (strstore 2 "process_spawner/1: ")

    .name: 1 limit
    echo (arg 1 0)
    print (strstore 2 " processs to launch")

    .name: 3 counter
    izero counter

    .mark: loop
    branch (igte 4 counter limit) endloop +1

    frame ^[(param 0 counter)]
    process 5 run_in_a_process/1

    frame ^[(param 0 5)]
    msg 0 detach/1

    iinc counter
    jump loop
    .mark: endloop

    return
.end

.function: main/1
    frame ^[(param 0 (istore 1 10))]
    process 1 process_spawner/1

    frame ^[(param 0 1) (param 1 (istore 2 512))]
    msg 0 setPriority/2

    join 0 1

    print (strstore 2 "main/1 exited")

    izero 0
    return
.end
