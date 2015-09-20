.signature: std::random::randint

.function: main
    ; FIXME: implement this chapter of "The Rust Book": https://doc.rust-lang.org/book/guessing-game.html
    import "random"

    ; random integer between 0 and 100
    frame ^[(param 0 (istore 2 0)) (param 1 (istore 2 100))]
    print (call 1 std::random::randint)

    izero 0
    end
.end
