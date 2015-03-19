.def: hey_im_absolute 0
    strstore 1 "Hey babe, I'm absolute."
    print 1
    izero 0
    end
.end

.def: main 0
    strstore 1 "I'm relative..."
    istore 1 1
    branch 1 :0

    print 1
    izero 0
    end
.end
