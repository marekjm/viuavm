.function: greetings
    echo (strstore 1 "Hello ")
    echo (arg 1 0)
    print (strstore 1 '!')
    return
.end

.signature: std::functional::apply

.function: main
    link std::functional

    function 1 greetings

    frame ^[(param 0 1) (param 1 (strstore 2 "World"))]
    call std::functional::apply

    frame ^[(param 0 1) (param 1 (strstore 2 "Joe"))]
    call std::functional::apply

    frame ^[(param 0 1) (param 1 (strstore 2 "Mike"))]
    call std::functional::apply

    izero 0
    return
.end
