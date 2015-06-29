.function: closure_a
    ; it has to be 2, because 2 register has been bound
    print 2
    end
.end

.function: closure_b
    arg 0 1

    ; overwrite bound value with whatever we got
    istore 2 @1
    end
.end

.function: returns_closures
    ; create a vector to store closures
    vec 1

    ; create a value to be bound in both closures
    istore 2 42

    ; create two closures binding the same variable
    ; presto, we have two functions that are share some state
    clbind 2
    closure closure_a 3
    clbind 2
    closure closure_b 4

    ; push closures to vector...
    vpush 1 3
    vpush 1 4

    ; ...and return the vector
    ; vectors can be used to return multiple values as
    ; they can hold any Type-derived type
    move 1 0
    end
.end

.function: main
    frame 0
    call returns_closures 1

    frame 0 0
    vat 1 2 0
    fcall 2 0

    istore 4 69

    frame 1 0
    param 0 4

    vat 1 3 1
    fcall 3 0

    frame 0 0
    fcall 2 0

    izero 0
    end
.end
