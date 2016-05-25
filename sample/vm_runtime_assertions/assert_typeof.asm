.block: try_TypeException
    frame ^[(param 0 (strstore 1 "Hello, World!")) (param 1 1)]
    print (msg 2 substr/)
    leave
.end

.block: catch_TypeException
    print (pull 6)
    leave
.end

.function: main
    try
    catch "TypeException" catch_TypeException
    enter try_TypeException

    izero 0
    return
.end
