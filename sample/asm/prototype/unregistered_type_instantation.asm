.block: handler
    print (pull 1)
    leave
.end

.block: unregistered_type_instantation
    new 1 Nonexistent
    leave
.end

.function: main
    try
    catch "Exception" handler
    enter unregistered_type_instantation

    izero 0
    return
.end
