.signature: std::random::device::urandom

.function: main
    ; first, import the random module to make std::random functions available
    import "random"

    ; this is a nonblocking call
    ; if gathered entropy bytes are not sufficient to form an integer,
    ; pseudo-random bytes will be used
    frame 0
    print (call 1 std::random::device::urandom)

    izero 0
    return
.end
