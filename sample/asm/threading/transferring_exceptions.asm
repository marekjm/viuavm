.function: run_in_a_thread
    ; will cause a memory leak on detached threads
    throw (threceive 1)
    end
.end

.block: try_thread_exception
    thjoin 1
    leave
.end

.block: handle_thread_exception
    echo (strstore 3 "exception transferred from thread ")
    echo 1
    echo (strstore 3 ": ")
    print (pull 3)
    leave
.end

.function: main
    frame 0
    thread 1 run_in_a_thread

    ;frame ^[(param 0 1)]
    ;msg 0 detach

    frame ^[(param 0 1) (param 1 (istore 2 40))]
    msg 0 setPriority

    frame ^[(param 0 1) (param 1 (strstore 2 "Hello exception transferring World!"))]
    msg 0 pass

    try
    catch "String" handle_thread_exception
    enter try_thread_exception

    izero 0
    end
.end
