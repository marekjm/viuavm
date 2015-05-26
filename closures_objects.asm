.def: set_x 0
    arg 0 2
    istore 1 @2
    end
.end

.def: get_x 1
    copy 1 0
    end
.end


.def: object 1
    arg 0 1

    clbind 1
    closure set_x 3

    clbind 1
    closure get_x 4

    vec 0
    vpush 0 3
    vpush 0 4

    end
.end


.def: main 1
    istore 1 42
    print 1

    ; get closure-based object
    frame 1
    param 0 1
    call object 2

    ; print it
    print 2

    ; extract method "get_x" from the object
    .name: 3 get_x
    vat 2 get_x 1
    print get_x

    clframe 0
    clcall get_x 5
    print 5


    ; extract method "set_x" from the object
    .name: 4 set_x
    vat 2 set_x 0
    print set_x

    ; set object's value to 69 (it was 42 before) using the "set_x" closure-method...
    istore 6 69
    clframe 1
    param 0 6
    clcall set_x 0

    ; then, call "get_x" once more to see that the value object is holding was really updated...
    clframe 0
    clcall get_x 5
    print 5


    izero 0
    end
.end
