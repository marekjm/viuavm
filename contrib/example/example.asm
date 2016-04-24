; signatures from foreign library
.signature: example::hello_world
.signature: example::hello
.signature: example::what_time_is_it

.block: __catch_0_main_Exception
    echo (strstore 1 "fail: ")
    print (pull 1)
    leave
.end
.block: __try_0_main
    frame 0
    call example::hello_world
    leave
.end

.function: main
    ; the foreign library must be imported
    import "example"


    ; exceptions thrown by foreign libraries can be caught as usual
    try
    catch "Exception" __catch_0_main_Exception
    enter __try_0_main


    ; call with a parameter
    frame ^[(param 0 (strstore 1 "Joe"))]
    call example::hello


    ; call with a return value
    frame 0
    call 2 example::what_time_is_it
    echo (strstore 1 "The time (in seconds since epoch) is: ")
    print 2

    izero 0
    return
.end
