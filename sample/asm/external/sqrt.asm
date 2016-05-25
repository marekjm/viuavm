.signature: math::sqrt/1

.function: main
    import "build/test/math"

    frame ^[(param 0 (fstore 1 3.0))]
    call 2 math::sqrt/1
    print 2

    izero 0
    return
.end
