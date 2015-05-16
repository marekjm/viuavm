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

.def: main 1
    tryframe
    catch "Exception" exception_handler
    try print_block

    izero 0
    end
.end
