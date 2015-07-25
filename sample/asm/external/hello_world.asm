.signature: World::print_hello

.function: main
    import "build/test/World"

    frame 0
    call 0 World::print_hello

    izero 0
    end
.end
