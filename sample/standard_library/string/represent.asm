.signature: std::string::represent/1

.function: main/1
    ; this instruction links "std::string" module into running machine
    link std::string

    ; create new object to test stringification
    new 1 Object

    ; obtain string representation of the object
    ; note the pass-by-pointer used to avoid copying since
    ; we want to get string representation of exactly the same object
    frame ^[(param 0 (ptr 3 1))]
    call 2 std::string::represent/1

    ; this should print two, exactly same lines
    print 1
    print 2

    izero 0
    return
.end
