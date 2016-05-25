.function: running_detached/0
.name: 0 counter
.name: 2 limit
    izero counter
    istore limit 4
    strstore 1 "Hello World! (from long-running detached process) "
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
    process 1 running_detached/0

    nop
    nop

    frame ^[(param 0 (ptr 2 1))]
    msg 0 detach/1

    nop

    ; reuse the pointer created earlier
    frame ^[(param 0 2)]
    msg 3 joinable/1
    print 3

    print (strstore 3 "main/1 exited")

    izero 0
    return
.end
