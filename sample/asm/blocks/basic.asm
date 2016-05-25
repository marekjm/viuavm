.block: main_block
    istore 1 42
    leave
.end

.function: main/1
    try
    enter main_block
    ; leave instructions lead here
    print 1

    izero 0
    return
.end
