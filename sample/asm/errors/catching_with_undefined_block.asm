.block: main/0__try
    throw (istore 1 42)
    leave
.end

.function: main/0
    try
    catch "Integer" main/0__catch
    enter main/0__try

    izero 0
    return
.end
