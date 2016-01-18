.signature: std::io::readtext

.function: run_in_a_thread
    .name: 1 counter
    istore counter 140
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

    frame ^[(param 0 (strstore 3 "./big_file.txt"))]
    call 4 std::io::readtext

    ;print 4
    print (strstore 5 "main/1 returning")

    izero 0
    return
.end
