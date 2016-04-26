; this program requires standard "typesystem" module to be available
.signature: typesystem::typeof


.function: typesystem_setup
    register (attach (class 1 Base) fn_base good_day)
    register (attach (derive (class 1 Derived) Base) fn_derived hello)
    register (attach (derive (class 1 MoreDerived) Derived) fn_more_derived hi)

    return
.end

.function: fn_base
    echo (strstore 1 "Good day from ")
    frame ^[(param 0 (arg 2 0))]
    print (call 3 typesystem::typeof)
    return
.end

.function: fn_derived
    echo (strstore 1 "Hello from ")
    frame ^[(param 0 (arg 2 0))]
    print (call 3 typesystem::typeof)
    return
.end

.function: fn_more_derived
    echo (strstore 1 "Hi from ")
    frame ^[(param 0 (arg 2 0))]
    print (call 3 typesystem::typeof)
    return
.end

.function: main
    import "typesystem"

    ; setup the typesystem
    call (frame 0) typesystem_setup

    ; create a Derived object and
    ; call methods on it
    new 1 Derived

    ; Good day from Derived
    frame ^[(param 0 1)]
    msg 0 good_day

    ; hello from Derived
    frame ^[(param 0 1)]
    msg 0 hello

    ; print an empty line
    print (strstore 3 "")

    ; create a MoreDerived object and
    ; call methods on it
    new 2 MoreDerived

    ; Good day from MoreDerived
    frame ^[(param 0 2)]
    msg 0 good_day

    ; Hello from MoreDerived
    frame ^[(param 0 2)]
    msg 0 hello

    ; Hi from MoreDerived
    frame ^[(param 0 2)]
    msg 0 hi

    izero 0
    return
.end
