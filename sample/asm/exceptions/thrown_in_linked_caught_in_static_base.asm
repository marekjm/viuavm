.signature: test_module::test_fun/0

.block: try_calling
    frame 0
    call test_module::test_fun/0
    leave
.end
.block: handle_Integer
    pull 1
    echo (strstore 2 "looks ")

    branch 1 +2
    strstore 2 "falsey"
    strstore 2 "truthy"

    echo 2
    echo (strstore 2 ": ")
    print 1

    leave
.end

.function: main/0
    link test_module

    try
    catch "Integer" handle_Integer
    enter try_calling

    izero 0
    return
.end
