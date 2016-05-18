.function: greetings/1
    echo (strstore 1 "Hello ")
    echo (arg 1 0)
    print (strstore 1 '!')
    return
.end

.signature: std::functional::apply/2

.function: main
    link std::functional

    function 1 greetings/1

    frame ^[(param 0 1) (param 1 (strstore 2 "World"))]
    call std::functional::apply/2

    frame ^[(param 0 1) (param 1 (strstore 2 "Joe"))]
    call std::functional::apply/2

    frame ^[(param 0 1) (param 1 (strstore 2 "Mike"))]
    call std::functional::apply/2

    izero 0
    return
.end
