.function: run_this_in_a_process/1
    print (strstore 1 "hi, I am process 0")
    return
.end

.function: process_waking_up_the_other_one/1
    print (strstore 1 "hi, I am process 1")

    print (strstore 6 "waking up process 0")
    frame ^[(param 0 (arg 2 0))]
    msg 0 wakeup/1

    return
.end

.function: main/1
    izero 1

    frame ^[(param 0 1)]
    process 2 run_this_in_a_process/1
    frame ^[(param 0 (ptr 3 2))]
    msg 0 detach/1

    print (strstore 6 "suspending process 0")
    frame ^[(param 0 3)]
    msg 0 suspend/1

    frame ^[(param 0 3)]
    process 4 process_waking_up_the_other_one/1

    join 0 4

    izero 0
    return
.end
