.function: watchdog_process/0
    .mark: watchdog_start
    throw (remove 4 (receive 1) (strstore 3 "function"))

    ;frame ^[(param 0 (ptr 2 1)) (param 1 (strstore 3 "function"))]
    ;msg 4 get

    ;echo (strstore 5 "process spawned with <")
    ;echo 4
    ;print (strstore 5 "> died")

    ;jump watchdog_start

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

.function: main/1
    frame 0
    watchdog watchdog_process/0

    frame 0
    process 1 broken_process/0
    frame ^[(param 0 (ptr 2 1))]
    msg 0 detach/1

    izero 0
    return
.end
