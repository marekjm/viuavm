.function: worker
    print (threceive 2)
    print (strstore 1 "worker: exiting")
    return
.end

.signature: std::io::getline

.function: producer
    frame 0
    call 2 std::io::getline
    print (strstore 1 "producer: exiting")
    return
.end

.function: main
    import "io"

    frame 0
    thread 1 worker
    ptr 2 1
    frame ^[(param 0 2)]
    msg 0 detach

    frame 0
    thread 3 producer
    ptr 4 3
    frame ^[(param 0 4)]
    msg 0 detach

    izero 0
    return
.end
