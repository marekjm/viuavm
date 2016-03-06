.function: broken_process
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

.signature: undefined_function

.function: main
    frame 0
    watchdog undefined_function

    frame 0
    process 1 broken_process
    frame ^[(param 0 (ptr 2 1))]
    msg 0 detach

    izero 0
    return
.end
