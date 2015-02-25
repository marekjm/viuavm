.link: jumplib.asm.wlib

.def: main 1
    istore 1 42
    frame 1
    param 0 1
    call jumprint 0
    izero 0
    end
.end
