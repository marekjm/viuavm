.block: try_ArityException
    frame ^[(param 0 (strstore 1 "Hello, World!")) (param 1 (istore 2 3)) (param 2 (istore 3 4)) (param 3 (istore 4 5))]
    print (msg 4 substr/)
    leave
.end

.block: catch_ArityException
    print (pull 6)
    leave
.end

.function: main/1
    try
    catch "ArityException" catch_ArityException
    enter try_ArityException

    izero 0
    return
.end
