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
    try
    catch "String" catcher
    enter second
    leave
.end

.block: second
    try
    catch "Vector" catcher
    enter third
    leave
.end

.block: third
    try
    catch "Float" catcher
    enter fourth
    leave
.end

.block: fourth
    try
    catch "Boolean" catcher
    enter this_throws
    leave
.end

.block: this_throws
    frame 0
    call foo
    end
.end

.function: main
    try
    catch "Integer" catcher
    enter first

    try
    catch "Integer" catcher
    enter this_throws

    izero 0
    end
.end
