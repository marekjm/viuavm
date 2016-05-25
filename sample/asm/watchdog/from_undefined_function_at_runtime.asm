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

.signature: undefined_function/0

.function: main/1
    frame 0
    watchdog undefined_function/0

    frame 0
    process 1 broken_process/0
    frame ^[(param 0 (ptr 2 1))]
    msg 0 detach/1

    izero 0
    return
.end
