.function: foo/0
    frame %0
    defer bar/0
    print (text %iota local "0. before deferred") local
    return
.end

.function: bar/0
    print (text %iota local "1. in deferred") local
    return
.end

.function: main/0
    frame %0
    call void foo/0
    print (text %iota local "2. after deferred") local

    izero %0 local
    return
.end
