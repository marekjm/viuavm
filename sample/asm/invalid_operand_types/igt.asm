.function: main
    register (class 1 Foo)
    new 1 Foo

    register (class 2 Bar)
    new 2 Bar
    igt 3 1 2

    izero 0
    return
.end
