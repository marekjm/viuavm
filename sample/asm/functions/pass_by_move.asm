.function: foo
    move 0 (print (arg 1 0))
    end
.end

.function: main
    print (new 1 Object)

    frame ^[(pamv 0 1)]
    print (call 1 foo)

    izero 0
    end
.end
