.signature: std::kitchensink::sleep/1

.function: main/1
    import "kitchensink"

    frame ^[(param 0 (istore 0 2))]
    call std::kitchensink::sleep/1

    izero 0
    return
.end
