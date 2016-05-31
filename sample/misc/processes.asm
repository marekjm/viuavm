.function: run_in_a_process/1
    echo (strstore 1 "spawned process ")
    echo (arg 1 0)
    print (strstore 1 " exiting")
    return
.end

.function: spawn_processes/2
    .name: 1 counter
    arg counter 0

    .name: 2 limit
    arg limit 1

    ; if limit is N, processes IDs go from 0 to N-1
    ; this is why the counter is incremented before the
    ; branch instruction which checks for equality
    iinc counter

    ; if the counter is equal to limit return immediately
    branch (ieq 3 counter limit) +1 +2
    return

    ; spawn a new process
    frame ^[(param 0 counter)]
    process 5 run_in_a_process/1

    frame ^[(param 0 5)]
    msg 0 detach/1

    ; take advantage of tail recursion in Viua and
    ; elegantly follow with process spawner execution
    frame ^[(pamv 0 counter) (pamv 1 limit)]
    tailcall spawn_processes/2

    ; FIXME: assembler should not complain if a function ends with tailcall
    ; instead of return
    return
.end

.function: process_spawner/1
    echo (strstore 2 "process_spawner/1: ")
    .name: 1 limit
    echo (arg 1 0)
    print (strstore 2 " processs to launch")

    frame ^[(pamv 0 (izero 3)) (pamv 1 limit)]
    tailcall spawn_processes/2

    return
.end

.function: main/1
    frame ^[(param 0 (istore 1 100000))]
    process 1 process_spawner/1

    frame ^[(param 0 1) (param 1 (istore 2 512))]
    msg 0 setPriority/2

    join 0 1

    print (strstore 2 "main/1 exited")

    izero 0
    return
.end
