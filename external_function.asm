.def: main 1
    strstore 1 "Hello World!"
    frame 1
    param 0 1
    excall sample_external_function 2

    print 2

    izero 0
    end
.end
