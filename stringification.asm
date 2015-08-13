.signature: viua::core::string::stringify

.function: main
    link string

    ; store an object...
    new 1 Object
    ; ...and print it, why not?
    print 1

    ; but!
    ; what about if wanted to store the string used to print the object?
    ; we would then call 'stringify/1' with the object as a parameter.
    ;
    ; let's do this!
    frame ^[(paref 0 1)]
    call 2 viua::core::string::stringify

    ; the output will be *identical* to the first print instruction
    ; it would be different if we passed the Object by value
    print 2

    izero 0
    end
.end
