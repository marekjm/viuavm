.def: closure_a 0
    ; it has to be 2, because 2 register has been bound
    print 2
    end
.end

.def: closure_b 0
    arg 0 1

    ; overwrite bound value with whatever we got
    istore 2 @1
    end
.end

.def: returns_closures 1
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

.def: main 1
    frame 0
    call returns_closures 1

    clframe 0
    vat 1 2 0
    clcall 2 0

    istore 4 69

    clframe 1
    param 0 4

    vat 1 3 1
    clcall 3 0

    clframe 0
    clcall 2 0

    izero 0
    end
.end
