.function: watchdog_process
    .mark: __begin
    .name: 1 death_message
    receive death_message

    .name: 2 exception
    remove exception 1 (strstore exception "exception")

    .name: 3 function
    remove function 1 (strstore function "function")

    echo (strstore 4 "[WARNING] process '")
    echo function
    echo (strstore 4 "' killed by >>>")
    echo exception
    print (strstore 4 "<<<")

    jump __begin

    return
.end

.function: a_detached_concurrent_process
    frame ^[(pamv 0 (istore 1 32))]
    call std::misc::cycle/1

    print (strstore 1 "Hello World (from detached process)!")

    frame ^[(pamv 0 (istore 1 512))]
    call std::misc::cycle/1

    print (strstore 1 "Hello World (from detached process) after a runaway exception!")

    frame ^[(pamv 0 (istore 1 512))]
    call std::misc::cycle/1

    frame ^[(pamv 0 (strstore 1 "a_detached_concurrent_process"))]
    call log_exiting_detached

    return
.end

.function: a_joined_concurrent_process
    frame ^[(pamv 0 (istore 1 128))]
    call std::misc::cycle/1

    print (strstore 1 "Hello World (from joined process)!")

    frame ^[(pamv 0 (strstore 1 "a_joined_concurrent_process"))]
    call log_exiting_joined

    throw (strstore 2 "OH NOES!")

    return
.end

.function: log_exiting_main
    print (strstore 2 "process [  main  ]: 'main' exiting")
    return
.end
.function: log_exiting_detached
    arg 1 0
    echo (strstore 2 "process [detached]: '")
    echo 1
    print (strstore 2 "' exiting")
    return
.end
.function: log_exiting_joined
    arg 1 0
    echo (strstore 2 "process [ joined ]: '")
    echo 1
    print (strstore 2 "' exiting")
    return
.end

.signature: std::misc::cycle/1

.function: main
    link std::misc

    frame 0
    watchdog watchdog_process

    frame 0
    process 1 a_detached_concurrent_process
    frame ^[(param 0 (ptr 2 1))]
    msg 0 detach
    ; delete the pointer to detached process
    delete 2

    frame 0
    process 2 a_joined_concurrent_process
    join 0 2

    frame 0
    call log_exiting_main

    izero 0
    return
.end
