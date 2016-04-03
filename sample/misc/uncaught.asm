.block: this_throws
    istore 1 42
    throw 1
    leave
.end

.def: main 1
    tryframe
    try this_throws

    izero 0
    end
.end
