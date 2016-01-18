.signature: std::io::getline

.function: run_in_a_thread
    .name: 1 counter
    istore counter 1400000
    strstore 2 "iterations left: "

    .mark: loop_begin
    branch counter +1 loop_end
    echo 2
    print counter
    idec counter
    jump loop_begin
    .mark: loop_end

    print (strstore 3 "run_in_a_thread/0 returned")

    return
.end

.function: main
    import "io"

    frame 0
    thread 1 run_in_a_thread
    frame ^[(param 0 (ptr 2 1))]
    msg 0 detach

    frame 0
    print (call 4 std::io::getline)

    print (strstore 5 "main/1 returning")

    izero 0
    return
.end
