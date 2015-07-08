.function: foo
    istore 1 666
    throw 1
    ;frame 0
    ;call bar
    end
.end

.function: bar
    frame 0
    call baz
    end
.end

.function: baz
    frame 0
    call bay
    end
.end

.function: bay
    frame 0
    call bax
    end
.end

.function: bax
    frame 0
    call baw
    end
.end

.function: baw
    frame 0
    call bau
    end
.end

.function: bau
    istore 1 69
    throw 1
    end
.end

.block: catcher
    pull 2
    strstore 3 "caught: "
    echo 3
    print 2
    leave
.end

.block: first
    tryframe
    catch "String" catcher
    try second
    leave
.end

.block: second
    tryframe
    catch "Vector" catcher
    try third
    leave
.end

.block: third
    tryframe
    catch "Float" catcher
    try fourth
    leave
.end

.block: fourth
    tryframe
    catch "Boolean" catcher
    try this_throws
    leave
.end

.block: this_throws
    frame 0
    call foo
    end
.end

.function: main
    tryframe
    catch "Integer" catcher
    try first

    tryframe
    catch "Integer" catcher
    try this_throws

    izero 0
    end
.end
