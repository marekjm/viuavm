; this program requires standard "typesystem" module to be available
.signature: typesystem::typeof


.function: typesystem_setup/0
    register (attach (class 1 Base) Base::saySomething/1 hello)
    register (attach (derive (class 1 Derived) Base) Derived::saySomethingMore/1 hello)

    return
.end

.function: Base::saySomething/1
    print (strstore 1 "Hello Base World!")
    return
.end

.function: Derived::saySomethingMore/1
    print (strstore 1 "Hello Derived World!")
    return
.end

.function: main
    call (frame 0) typesystem_setup/0

    ; create a Base object and
    ; send a message to it
    frame ^[(param 0 (new 1 Base))]
    msg 0 hello

    ; create a Derived object and
    ; send a message to it
    frame ^[(param 0 (new 2 Derived))]
    msg 0 hello

    frame ^[(param 0 2)]
    call 0 Base::saySomething/1

    izero 0
    return
.end
