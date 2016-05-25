.signature: std::random::device::random

.function: main/1
    ; first, import the random module to make std::random functions available
    import "random"

    ; this call can block if not enough entropy bytes can be found
    ; to form an integer, but is safe
    frame 0
    print (call 1 std::random::device::random)

    izero 0
    return
.end
