.signature: std::random::random

.function: main
    ; first, import the random module to make std::random functions available
    import "random"

    ; returns random float
    ; probably the most often used function from std::random
    frame 0
    print (call 1 std::random::random)

    izero 0
    end
.end
