.def: hey_im_absolute 0
    strstore 1 "Hey babe, I'm absolute."
    ; FIXME: uncomment next line when absolute forward-jumps are implemented
    ;jump :5
    print 1
    izero 0
    end
.end

.def: main 0
    strstore 1 "My morals are relative."
    jump :0
    print 1
    izero 0
    end
.end
