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

.function: watchdog_process
    .mark: start_watching
    receive 1

    print 1

    jump start_watching
    return
.end

.function: main
    frame 0
    watchdog watchdog_process

    import "build/test/throwing"

    try
    catch "Exception" __catch_Exception
    enter __try

    izero 0
    return
.end
