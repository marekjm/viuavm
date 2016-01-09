; this program requires standard "typesystem" module to be available
.signature: typesystem::typeof


.function: typesystem_setup
    register (attach (class 1 Base) Base::saySomething hello)
    register (attach (derive (class 1 Derived) Base) Derived::saySomethingMore hello)

    end
.end

.function: Base::saySomething
    print (strstore 1 "Hello Base World!")
    end
.end

.function: Derived::saySomethingMore
    print (strstore 1 "Hello Derived World!")
    end
.end

.function: main
    call (frame 0) typesystem_setup

    ; create a Base object and
    ; send a message to it
    frame ^[(param 0 (new 1 Base))]
    msg 0 hello

    ; create a Derived object and
    ; send a message to it
    frame ^[(param 0 (new 2 Derived))]
    msg 0 hello

    frame ^[(param 0 2)]
    call 0 Base::saySomething

    izero 0
    end
.end
