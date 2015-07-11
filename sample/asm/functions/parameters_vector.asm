.bsignature: misc::argsvector

.function: foo
    link misc

    ; FIXME: tryframe will not be needed when ENTER instruction is implemented
    tryframe
    try misc::argsvector

    print 1
    end
.end

.function: main
    izero 1
    frame 4

    param @1 1
    iinc 1

    param @1 1
    iinc 1

    param @1 1
    iinc 1

    param @1 1
    iinc 1

    call foo

    izero 0
    end
.end
