.function: run_in_a_thread
    echo (strstore 1 "spawned thread ")
    echo (arg 1 0)
    print (strstore 1 " exiting")
    end
.end

.function: thread_spawner
    echo (strstore 2 "thread_spawner: ")
    echo (istore 1 1000000)
    print (strstore 2 " threads to launch")

.name: 1 limit
.name: 3 counter
    izero counter
.mark: loop
    branch (igte 4 counter limit) endloop +1

    frame ^[(param 0 counter)]
    thread 5 run_in_a_thread

    frame ^[(param 0 5)]
    msg 0 detach/1

    iinc counter
    jump loop
.mark: endloop
    end
.end

.function: main
    frame ^[(param 0 (istore 1 1000000))]
    thread 1 thread_spawner

    frame ^[(param 0 1) (param 1 (istore 2 512))]
    msg 0 setPriority/2

    thjoin 1

    print (strstore 2 "main/1 exited")

    izero 0
    end
.end
