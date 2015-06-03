.def: typeof 1
    arg 0 1
    frame 1
    param 0 1
    excall typesystem.typeof 2
    move 2 0
    end
.end

.def: inheritanceChain 1
    arg 0 1
    frame 1
    param 0 1
    excall typesystem.inheritanceChain 2
    move 2 0
    end
.end

.def: bases 1
    arg 0 1
    frame 1
    param 0 1
    excall typesystem.bases 2
    move 2 0
    end
.end


.block: handle_LinkException
    pull 2
    print 2
    halt
.end

.block: use
    eximport "typesystem"
    leave
.end

.def: main 1
    istore 1 42
    print 1

    ; placeholder, for now it is not possible to use strings from registers
    strstore 2 "typesystem"
    tryframe
    catch "LinkException" handle_LinkException
    catch "Exception" handle_LinkException
    try use

    frame 1
    paref 0 1
    call typeof 2
    print 2

    frame 1
    paref 0 1
    call bases 2
    print 2

    frame 1
    paref 0 1
    call inheritanceChain 2
    print 2


    izero 0
    end
.end
