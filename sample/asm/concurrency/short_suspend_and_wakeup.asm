.function: run_this_in_a_thread
    print (strstore 1 "hi, I am thread 0")
    return
.end

.function: thread_waking_up_the_other_one
    print (strstore 1 "hi, I am thread 1")

    print (strstore 6 "waking up thread 0")
    frame ^[(param 0 (arg 2 0))]
    msg 0 wakeup

    return
.end

.function: main
    izero 1

    frame ^[(param 0 1)]
    thread 2 run_this_in_a_thread
    frame ^[(param 0 (ptr 3 2))]
    msg 0 detach

    print (strstore 6 "suspending thread 0")
    frame ^[(param 0 3)]
    msg 0 suspend

    frame ^[(param 0 3)]
    thread 4 thread_waking_up_the_other_one

    thjoin 0 4

    izero 0
    return
.end
