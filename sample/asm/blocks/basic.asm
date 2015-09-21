.block: main_block
    istore 1 42
    leave
.end

.function: main
    tryframe
    enter main_block
    ; leave instructions lead here
    print 1

    izero 0
    end
.end
