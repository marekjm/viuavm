.function: running_detached
.name: 0 counter
.name: 2 limit
    izero counter
    istore limit 4
    strstore 1 "Hello World! (from long-running detached thread) "
.mark: loop
    branch (igte 3 counter limit) after_loop
    echo 1
    print counter
    iinc counter
    jump loop
.mark: after_loop
    return
.end

.function: main
    frame 0
    thread 1 running_detached

    nop
    nop

    frame ^[(paref 0 1)]
    msg 0 detach

    nop

    frame ^[(paref 0 1)]
    msg 3 joinable
    print 3

    print (strstore 3 "main/1 exited")

    izero 0
    return
.end
