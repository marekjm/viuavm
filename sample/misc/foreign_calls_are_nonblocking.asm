.signature: std::io::getline/0
.signature: std::misc::cycle/1

.function: run_in_a_process/0
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

.function: main/1
    import "io"
    link std::misc

    frame 0
    process 1 run_in_a_process/0

    frame ^[(param 0 (ptr 2 1))]
    msg 0 detach/1
    print (strstore 2 "main/1 detached worker process")

    echo (strstore 2 "message to pass: ")
    frame 0
    call 2 std::io::getline/0

    frame ^[(param 0 1) (param 1 2)]
    msg 0 pass/2

    izero 0
    return
.end
