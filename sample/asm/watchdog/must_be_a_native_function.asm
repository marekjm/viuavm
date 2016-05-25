.signature: World::print_hello/0

.function: main/1
    import "build/test/World"

    frame 0
    watchdog World::print_hello/0

    izero 0
    return
.end
