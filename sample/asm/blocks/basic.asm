.block: main_block
    istore 1 42
    leave
.end

.def: main 1
    tryframe
    try main_block
    ; leave instructions lead here
    print 1

    izero 0
    end
.end
