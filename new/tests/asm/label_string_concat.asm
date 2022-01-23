.label: hello_world
; The line continuation is there to test whether it works OK.
.value: string "Hello" \
               ", " "World!"

.function: [[entry_point]] main
    string $1, @hello_world
    ebreak
    return
.end
