.function: valid/1
    print (arg 1 0)
    return
.end

.function: another_valid/0
    frame ^[(pamv 0 (strstore 1 "Hello World!"))]
    tailcall valid/1
.end

.function: foo/x
    return
.end

.function: main
    izero 0
    return
.end
