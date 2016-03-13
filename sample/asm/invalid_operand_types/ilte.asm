.function: main
    strstore 1 "Hello"

    register (class 2 Foo)
    new 2 Foo
    ilte 3 2 1

    izero 0
    return
.end
