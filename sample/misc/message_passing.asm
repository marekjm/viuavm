.function: consumer/0
    print (receive 2)
    print (strstore 1 "consumer/0: exiting")
    return
.end

.signature: std::io::getline/0

.function: producer/1
    ; the producer will read users input and
    ; send it to the consumer
    frame 0
    call 2 std::io::getline/0

    print (strstore 1 "producer/1: exiting")

    frame ^[(param 0 (arg 3 0)) (param 1 2)]
    msg 0 pass/2

    return
.end

.function: main/1
    import "io"

    ; spawn the consumer process...
    frame 0
    process 1 consumer/0

    ; ...and detach it, so main/1 exiting will not
    ; kill it
    frame ^[(param 0 (ptr 2 1))]
    msg 0 detach/1

    ; spawn the producer process and pass consumer process handle to it
    ; producer needs to know what consumer it must pass a message to
    ;
    ; pass-by-move to avoid copying, the handle will not be used by main/1 at
    ; any later point
    frame ^[(pamv 0 1)]
    process 3 producer/1

    ; detach producer process
    frame ^[(param 0 (ptr 4 3))]
    msg 0 detach/1

    ; main/1 may safely exit
    izero 0
    return
.end
