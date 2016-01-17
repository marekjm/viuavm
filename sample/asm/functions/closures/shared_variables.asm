.function: closure_a
    ; it has to be 2, because 2 register has been bound
    print 2
    return
.end

.function: closure_b
    arg 1 0

    ; overwrite bound value with whatever we got
    istore 2 @1
    return
.end

.function: returns_closures
    ; create a vector to store closures
    vec 1

    ; create a value to be bound in both closures
    istore 2 42

    ; create two closures binding the same variable
    ; presto, we have two functions that are share some state
    clbind 2
    closure 3 closure_a
    clbind 2
    closure 4 closure_b

    ; push closures to vector...
    vpush 1 3
    vpush 1 4

    ; ...and return the vector
    ; vectors can be used to return multiple values as
    ; they can hold any Type-derived type
    move 0 1
    return
.end

.function: main
    frame 0
    call 1 returns_closures

    frame 0 0
    vat 2 1 0
    fcall 0 2

    istore 4 69

    frame 1 0
    param 0 4

    vat 3 1 1
    fcall 0 3

    frame 0 0
    fcall 0 2

    izero 0
    return
.end
