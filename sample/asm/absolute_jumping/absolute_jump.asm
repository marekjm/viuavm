.function: hey_im_absolute/0
    strstore 1 "Hey babe, I'm absolute."
    jump .7
    print 1
    izero 0
    return
.end

.function: main/1
    strstore 1 "My morals are relative."
    jump .0
    print 1
    izero 0
    return
.end
