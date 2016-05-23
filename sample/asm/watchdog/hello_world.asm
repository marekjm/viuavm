.function: watchdog_process/0
    .mark: watchdog_start
    receive 1

    remove 4 1 (strstore 3 "function")

    echo (strstore 5 "process spawned with <")
    echo 4
    print (strstore 5 "> died")

    jump watchdog_start

    return
.end

.function: broken_process/0
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    throw (istore 1 42)
    return
.end

.function: main
    frame 0
    watchdog watchdog_process/0

    frame 0
    process 1 broken_process/0
    frame ^[(param 0 (ptr 2 1))]
    msg 0 detach

    izero 0
    return
.end
