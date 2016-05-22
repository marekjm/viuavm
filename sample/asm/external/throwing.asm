.signature: throwing::oh_noes

.block: __try
    frame 0
    call 0 throwing::oh_noes
    leave
.end
.block: __catch_Exception
    print (pull 1)
    leave
.end

.function: watchdog_process/0
    .mark: start_watching
    .name: 1 death_message
    receive death_message

    .name: 3 exception
    remove exception death_message (strstore exception "exception")
    print exception

    jump start_watching
    return
.end

.function: main
    frame 0
    watchdog watchdog_process/0

    import "build/test/throwing"

    try
    catch "ExceptionX" __catch_Exception
    enter __try

    izero 0
    return
.end
