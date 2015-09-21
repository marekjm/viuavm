.block: exception_handler
    strstore 1 "exception encountered: "
    pull 2
    echo 1
    print 2
    leave
.end

.block: print_block
    print 1
    leave
.end

.function: main
    try
    catch "Exception" exception_handler
    enter print_block

    izero 0
    end
.end
