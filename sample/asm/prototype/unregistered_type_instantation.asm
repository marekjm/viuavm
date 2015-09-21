.block: handler
    print (pull 1)
    leave
.end

.block: unregistered_type_instantation
    new 1 Nonexistent
    leave
.end

.function: main
    tryframe
    catch "Exception" handler
    enter unregistered_type_instantation

    izero 0
    end
.end
