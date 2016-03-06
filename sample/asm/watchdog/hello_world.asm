.function: watchdog_thread
    .mark: watchdog_start
    threceive 1

    frame ^[(param 0 (ptr 2 1)) (param 1 (strstore 3 "function"))]
    msg 4 get

    echo (strstore 5 "process spawned with <")
    echo 4
    print (strstore 5 "> died")

    jump watchdog_start

    return
.end

.function: broken_thread
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
    watchdog watchdog_thread

    frame 0
    process 1 broken_thread
    frame ^[(param 0 (ptr 2 1))]
    msg 0 detach

    izero 0
    return
.end
