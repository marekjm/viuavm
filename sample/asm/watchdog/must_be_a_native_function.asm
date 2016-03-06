.signature: World::print_hello

.function: main
    import "build/test/World"

    frame 0
    watchdog World::print_hello

    izero 0
    return
.end
