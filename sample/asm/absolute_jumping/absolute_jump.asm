.def: foo 0
    strstore 1 "Hello World!"
    print 1
    izero 0
    end
.end

.def: main 0
    jump :0
    izero 0
    end
.end
