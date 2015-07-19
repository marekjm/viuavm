.function: main
    fstore 1 3.0

    eximport "build/test/math"
    frame 1
    param 0 1
    excall 2 math::sqrt

    print 2

    izero 0
    end
.end
