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
    watchdog undefined_function

    frame 0
    thread 1 broken_thread
    frame ^[(param 0 (ptr 2 1))]
    msg 0 detach

    izero 0
    return
.end
