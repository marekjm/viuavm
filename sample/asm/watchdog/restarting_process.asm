.function: watchdog_process/0
    .mark: __begin
    .name: 1 death_message
    receive death_message

    .name: 2 exception
    remove exception 1 (strstore exception "exception")

    .name: 3 function
    remove function 1 (strstore function "function")

    .name: 4 parameters
    remove parameters 1 (strstore parameters "parameters")

    echo (strstore 5 "[WARNING] process '")
    echo function
    echo parameters
    echo (strstore 5 "' killed by >>>")
    echo exception
    print (strstore 5 "<<<")

    ; handle only a_division_executing_process
    frame ^[(param 0 (vat 5 parameters 0)) (param 1 (iinc (vat 6 parameters 1)))]
    ;call 0 a_division_executing_process
    process 7 a_division_executing_process/2
    frame ^[(param 0 (ptr 5 7))]
    msg 0 detach/1

    jump __begin

    return
.end

.function: a_detached_concurrent_process/0
    frame ^[(pamv 0 (istore 1 32))]
    call std::misc::cycle/1

    print (strstore 1 "Hello World (from detached process)!")

    frame ^[(pamv 0 (istore 1 512))]
    call std::misc::cycle/1

    print (strstore 1 "Hello World (from detached process) after a runaway exception!")

    frame ^[(pamv 0 (istore 1 512))]
    call std::misc::cycle/1

    frame ^[(pamv 0 (strstore 1 "a_detached_concurrent_process"))]
    call log_exiting_detached/1

    return
.end

.function: a_division_executing_process/2
    frame ^[(pamv 0 (istore 1 128))]
    call std::misc::cycle/1

    .name: 1 divide_what
    arg divide_what 0

    .name: 2 divide_by
    arg divide_by 1

    .name: 3 zero
    izero zero

    branch (ieq 4 divide_by zero) +1 __after_throw
    throw (strstore 4 "cannot divide by zero")
    .mark: __after_throw

    idiv 0 divide_what divide_by
    echo divide_what
    echo (strstore 4 ' / ')
    echo divide_by
    echo (strstore 4 ' = ')
    print 0

    return
.end

.function: log_exiting_main/0
    print (strstore 2 "process [  main  ]: 'main' exiting")
    return
.end
.function: log_exiting_detached/1
    arg 1 0
    echo (strstore 2 "process [detached]: '")
    echo 1
    print (strstore 2 "' exiting")
    return
.end
.function: log_exiting_joined/0
    arg 1 0
    echo (strstore 2 "process [ joined ]: '")
    echo 1
    print (strstore 2 "' exiting")
    return
.end

.signature: std::misc::cycle/1

.function: main/1
    link std::misc

    frame 0
    watchdog watchdog_process/0

    frame 0
    process 1 a_detached_concurrent_process/0
    frame ^[(param 0 (ptr 2 1))]
    msg 0 detach/1
    ; delete the pointer to detached process
    delete 2

    frame ^[(param 0 (istore 3 42)) (param 1 (istore 4 0))]
    process 2 a_division_executing_process/2
    frame ^[(param 0 (ptr 3 2))]
    msg 0 detach/1
    ; delete the pointer to detached process
    delete 3

    frame 0
    call log_exiting_main/0

    izero 0
    return
.end
