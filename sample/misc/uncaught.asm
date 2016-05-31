.block: this_throws
    istore 1 42
    throw 1
    leave
.end

.function: main/0
    try
    enter this_throws

    izero 0
    return
.end
