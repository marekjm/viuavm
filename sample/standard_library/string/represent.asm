.signature: std::string::represent

.function: main
    ; this instruction links "std::string" module into running machine
    link std::string

    ; create new object to test stringification
    new 1 Object

    ; obtain string representation of the object
    ; note the pass-by-reference used to avoid copying since
    ; we want to get string representation of exactly the same object
    frame ^[(paref 0 1)]
    call 2 std::string::represent

    ; this should print two, exactly same lines
    print 1
    print 2

    izero 0
    end
.end
