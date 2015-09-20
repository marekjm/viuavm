; std::random::randint/2(Integer lower_bound, Integer upper_bound) -> Integer
;   Returned integer is between lower and upper bound.
.signature: std::random::randint

.function: main
    ; first, import the random module to make std::random functions available
    import "random"

    ; random integer between 0 and 100
    frame ^[(param 0 (istore 2 0)) (param 1 (istore 2 100))]
    print (call 1 std::random::randint)

    izero 0
    end
.end
