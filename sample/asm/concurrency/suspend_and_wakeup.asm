.function: thread_to_suspend
    .name: 1 i
    istore 1 40
    .name: 2 m
    strstore 2 "iterations left (TTS): "
    .mark: __thread_to_suspend_begin_while_0

    branch 1 +1 __thread_to_suspend_end_while_0

    echo 2
    nop
    nop
    idec i 
    nop
    print 1
    jump __thread_to_suspend_begin_while_0

    .mark: __thread_to_suspend_end_while_0

    print (strstore 1 "thread_to_suspend/0 returned")
    return
.end

.function: thread_to_do_the_suspending
    frame ^[(param 0 (arg 1 0)]
    msg 0 detach

    istore 4 10
    strstore 5 "iterations left (TDD): "
    .mark: loop_begin_0
    branch 4 +1 loop_end_0
    echo 5
    nop
    nop
    nop
    nop
    idec 4
    print 4
    jump loop_begin_0
    .mark: loop_end_0

    frame ^[(param 0 1)]
    msg 0 suspend

    frame ^[(param 0 1]
    msg 2 suspended

    print 2
    istore 4 10
    strstore 5 "iterations left (TDD): "
    .mark: loop_begin
    branch 4 +1 loop_end
    echo 5
    nop
    nop
    nop
    nop
    idec 4
    print 4
    jump loop_begin
    .mark: loop_end

    frame ^[(param 0 1)]
    msg 0 wakeup

    frame ^[(param 0 1]
    msg 2 suspended
    print 2

    return
.end

.function: main
    frame 0
    thread 1 thread_to_suspend
    
    frame ^[(param 0 (ptr 2 1))]
    thread 3 thread_to_do_the_suspending
    thjoin 0 3

    print (strstore 5 "main/1 returned")
    izero 0
    return
.end
