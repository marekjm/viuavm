.function: foo
    istore 1 666
    throw 1
    end
.end

.block: catch_integer
    pull 2
    strstore 3 "caught: "
    echo 3
    print 2
    leave
.end

.block: foo_block
    ;istore 1 69
    ;throw 1
    frame 0
    call foo
    leave
.end

.function: main
    tryframe
    catch "Integer" catch_integer
    try foo_block

    izero 0
    end
.end
