.signature: std::io::getline

.function: run_in_a_thread
    print (strstore 1 "worker thread: starting...")
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    print (strstore 1 "worker thread: started")
    threceive 1
    print 1
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    print (strstore 1 "worker thread: stopped")
    end
.end

.function: main
    import "io"

    frame 0
    thread 1 run_in_a_thread

    frame ^[(param 0 1) (param 1 (istore 2 40))]
    msg 0 setPriority

    frame ^[(param 0 1)]
    msg 0 detach
    print (strstore 2 "main/1 detached worker thread")

    echo (strstore 2 "message to pass: ")
    frame 0
    call 2 std::io::getline

    frame ^[(param 0 1) (param 1 2)]
    msg 0 pass

    izero 0
    end
.end
