.block: handle_Exception
    strstore 2 "Exception occured: cannot obtain real type of object, assuming: Object"
    print 2
    strstore 2 "Object"
    leave
.end

.block: get_typeof
    eximport "typesystem"

    frame 1
    paref 0 1
    excall typesystem.typeof 2

    leave
.end

.def: typeof 1
    arg 0 1

    tryframe
    catch "Exception" handle_Exception
    try get_typeof

    move 2 0
    end
.end

.def: main 1
    istore 1 42
    print 1

    ; tryframe
    ; catch "Exception" handle_Exception
    ; try get_typeof

    ; print typeof
    frame 1
    paref 0 1
    call typeof 2
    print 2

    izero 0
    end
.end
