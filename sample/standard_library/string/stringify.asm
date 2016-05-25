.signature: std::string::stringify/1

.function: main/1
    ; this instruction links "std::string" module into running machine
    link std::string

    ; create new object to test stringification
    new 1 Object

    ; stringify the object
    ; note the pass-by-pointer used to avoid copying since
    ; we want to get stringification of exactly the same object
    frame ^[(param 0 (ptr 2 1))]
    call 3 std::string::stringify/1

    ; this should print two, exactly same lines
    print 1
    print 3

    izero 0
    return
.end
