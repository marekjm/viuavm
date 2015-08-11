.function: std::string::stringify
    strstore 1 ""
    frame ^[(paref 0 (arg 2 0))]
    msg 1 1 stringify
    move 0 1
    end
.end

.function: std::string::represent
    strstore 1 ""
    frame ^[(paref 0 (arg 2 0))]
    msg 1 1 represent
    move 0 1
    frame
    end
.end

.function: main
    strstore 1 "Hello World!"

    frame ^[(paref 0 1)]
    call 1 std::string::stringify

    frame ^[(paref 0 1)]
    call 2 std::string::represent

    print 1
    print 2

    izero 0
    end
.end
