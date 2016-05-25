.signature: std::io::getline
.signature: std::misc::cycle/1

.function: run_in_a_process
    print (strstore 1 "worker process: starting...")

    frame ^[(pamv 0 (istore 1 524288))]
    call std::misc::cycle/1

    print (strstore 1 "worker process: started")
    print (receive 1)

    frame ^[(pamv 0 (istore 1 524288))]
    call std::misc::cycle/1

    print (strstore 1 "worker process: stopped")
    return
.end

.function: main
    import "io"
    link std::misc

    frame 0
    process 1 run_in_a_process

    frame ^[(param 0 (ptr 2 1))]
    msg 0 detach/1
    print (strstore 2 "main/1 detached worker process")

    echo (strstore 2 "message to pass: ")
    frame 0
    call 2 std::io::getline

    frame ^[(param 0 1) (param 1 2)]
    msg 0 pass

    izero 0
    return
.end
