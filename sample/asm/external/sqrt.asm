.def: main
    fstore 1 3.0

    eximport "math"
    frame 1
    param 0 1
    excall math.sqrt 2

    print 2

    izero 0
    end
.end
