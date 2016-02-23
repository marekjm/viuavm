.function: set_x
    ; this is the setter method

    ; store 0th argument in 2nd register
    arg 2 0

    ; store an Integer in 1st register
    ; use value found in 2nd register as input
    istore 1 @2

    ; return from the function
    return
.end

.function: get_x
    ; this is the getter method

    ; simply copy value from 1st register into 0th register
    ; register 0 is used as a return value register so
    ; this method will return a copy of the Integer being held
    ; by the simulated closure-based object
    copy 0 1

    ; return from the function
    return
.end


.function: object
    ; this function acts as a constructor for the closure-based object
    ; 2nd register is intentionally left unused
    ; because the "set_x" closure uses it to store the argument it gets

    ; store 0th argument in 1st register
    arg 1 0

    ; tell the CPU to bind 1st register in a closure
    enclose 1
    ; create a closure from function "set_x" in 3rd register
    closure 3 set_x

    ; tell the CPU to bind 1st register in a closure
    ; this is required because CLOSURE opcode resets flags set
    ; by ENCLOSE opcode
    enclose 1
    ; create a closure from function "get_x" in 4th register
    closure 4 get_x

    ; this will be the return value of the our constructor
    ; create a vector in 0th register...
    vec 0
    ; ...and push closures into it
    vpush 0 3
    vpush 0 4

    ; return from the function
    return
.end


.function: main
    ; store Integer 42 in 1st register and
    ; print it
    istore 1 42
    print 1

    ; create a closure-based object using constructor function
    ; defined earlier
    frame 1
    param 0 1
    call 2 object

    ; print the object
    ; actually, second register is just a vector of closures so
    ; the object cannot be accessed directly - it is lost
    ; but because the machine saw the registers in constructor as *bound*
    ; it did not free the memory in them: so the value
    ; wrapped by the object is still available!
    print 2


    ; extract method "get_x" from the object
    ; set a name for the register (for readability purposes)
    .name: 3 get_x
    ; using vector in 2nd register,
    ; create a reference in register named 'get_x' that will point to
    ; index 1 in the vector being used
    vat get_x 2 1
    ; print what was returned
    print get_x

    ; create closure frame
    frame 0
    ; call the getter-closure
    ; the call will store a copy of the Intger that
    ; was passed to the constructor function
    fcall 5 get_x
    print 5


    ; extract method "set_x" from the object
    .name: 4 set_x
    vat set_x 2 0
    print set_x

    ; set object's value to 69 (it was 42 before) using the setter-closure
    istore 6 69
    frame 1
    ; set 0th parameter to be a copy of the object from 6th register
    param 0 6
    fcall 0 set_x

    ; then, call "get_x" once more to access the value stored by the
    ; simulated object
    ; remember - closure registers are bound so the call to "set_x" updated
    ; the same memory that "get_x" accesses
    frame 0
    fcall 5 get_x

    ; print what was returned and
    ; see the Integer is now 69
    print 5

    izero 0
    return
.end
