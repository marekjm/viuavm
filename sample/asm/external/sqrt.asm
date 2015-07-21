.signature: math::sqrt

.function: main
    fstore 1 3.0

    import "build/test/math"
    frame 1
    param 0 1
    call 2 math::sqrt

    print 2

    izero 0
    end
.end
